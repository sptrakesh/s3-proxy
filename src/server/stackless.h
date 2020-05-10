//
// Created by Rakesh on 05/05/2020.
//

#pragma once

#include "s3util.h"
#include "log/NanoLog.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace spt::server
{
  // Return a reasonable mime type based on the extension of a file.
  beast::string_view mime_type( beast::string_view path );

  // Append an HTTP rel-path to a local filesystem path.
  // The returned path is normalized for the platform.
  std::string path_cat( beast::string_view base, beast::string_view path );

  // This function produces an HTTP response for the given
  // request. The type of the response object depends on the
  // contents of the request, so the interface requires the
  // caller to pass a generic lambda for receiving the response.
  template<
      class Body, class Allocator,
      class Send>
  void
  handle_request(
      beast::string_view doc_root,
      http::request<Body, http::basic_fields<Allocator>>&& req,
      Send&& send, net::ip::address client )
  {
    // Returns a bad request response
    auto const bad_request =
        [&req]( beast::string_view why )
        {
          http::response<http::string_body> res{ http::status::bad_request,
              req.version() };
          res.set( http::field::server, BOOST_BEAST_VERSION_STRING );
          res.set( http::field::content_type, "text/html" );
          res.keep_alive( req.keep_alive());
          res.body() = std::string( why );
          res.prepare_payload();
          return res;
        };

    auto const not_allowed =
        [&req]()
        {
          http::response<http::string_body> res{ http::status::method_not_allowed,
              req.version() };
          res.set( http::field::server, BOOST_BEAST_VERSION_STRING );
          res.set( http::field::content_type, "text/html" );
          res.keep_alive( req.keep_alive() );
          res.prepare_payload();
          return res;
        };

    // Returns a not found response
    auto const not_found =
        [&req]( beast::string_view target )
        {
          http::response<http::string_body> res{ http::status::not_found,
              req.version() };
          res.set( http::field::server, BOOST_BEAST_VERSION_STRING );
          res.set( http::field::content_type, "text/html" );
          res.keep_alive( req.keep_alive());
          res.body() =
              "The resource '" + std::string( target ) + "' was not found.";
          res.prepare_payload();
          return res;
        };

    // Returns a server error response
    auto const server_error =
        [&req]( beast::string_view what )
        {
          http::response<http::string_body> res{
              http::status::internal_server_error, req.version() };
          res.set( http::field::server, BOOST_BEAST_VERSION_STRING );
          res.set( http::field::content_type, "text/html" );
          res.keep_alive( req.keep_alive());
          res.body() = "An error occurred: '" + std::string( what ) + "'";
          res.prepare_payload();
          return res;
        };

    // Returns a not modified response
    auto const not_modified = [&req]()
        {
          http::response<http::string_body> res{
              http::status::not_modified, req.version() };
          res.set( http::field::server, BOOST_BEAST_VERSION_STRING );
          res.keep_alive( req.keep_alive() );
          res.prepare_payload();
          return res;
        };

    // Return a forbidden response
    auto const forbidden = [&req]()
    {
      http::response<http::string_body> res{
          http::status::forbidden, req.version() };
      res.set( http::field::server, BOOST_BEAST_VERSION_STRING );
      res.keep_alive( req.keep_alive() );
      res.prepare_payload();
      return res;
    };

    // Make sure we can handle the method
    if ( req.method() != http::verb::get &&
        req.method() != http::verb::head &&
        req.method() != http::verb::options )
      return send( not_allowed() );

    auto ip = req["x-forwarded-for"];
    if ( ip.empty() ) ip = client.to_string();
    LOG_DEBUG << "Request from " << std::string_view{ ip.data(), ip.size() };

    if ( req.method() == http::verb::options )
    {
      http::response<http::empty_body> res{ http::status::no_content,
          req.version() };
      res.set( "Access-Control-Allow-Origin", "*" );
      res.set( "Access-Control-Allow-Methods", "GET" );
      res.set( http::field::server, BOOST_BEAST_VERSION_STRING );
      res.keep_alive( req.keep_alive() );
      return send( std::move( res ) );
    }

    if ( http::verb::get == req.method() && req.target() == "/_proxy/_private/_cache/clear" )
    {
      auto bearer = req["authorization"];
      if ( bearer.empty() ) bearer = req["Authorization"];
      if ( bearer.empty() ) return send( forbidden() );

      const auto prefix = std::string("Bearer ");
      if ( !boost::algorithm::starts_with( bearer, prefix ) )
      {
        return send( bad_request( "Invalid Authorization header" ) );
      }

      std::string temp{ bearer };
      temp.erase( 0, prefix.size() );

      return S3Util::instance().clear( temp ) ? send( not_modified() ) : send( forbidden() );
    }

    // Request path must be absolute and not contain "..".
    if ( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find( ".." ) != beast::string_view::npos )
      return send( bad_request( "Illegal request-target" ));

    std::string resource{ req.target().data(), req.target().size() };
    if ( req.target().back() == '/' ) resource.append( "index.html" );

    auto downloaded = S3Util::instance().get( resource );
    if ( ! downloaded || downloaded->fileName.empty() )
    {
      LOG_WARN << "Error downloading resource " << resource;
      return send( not_found( req.target() ) );
    }
    else
    {
      LOG_INFO << "Downloaded resource " << resource << " from S3\n" << downloaded->str();
    }

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open( downloaded->fileName.c_str(), beast::file_mode::scan, ec );

    // Handle the case where the file doesn't exist
    if ( ec == beast::errc::no_such_file_or_directory ) return send( not_found( req.target() ) );

    // Handle an unknown error
    if ( ec ) return send( server_error( ec.message() ) );

    // Cache the size since we need it after the move
    auto const size = downloaded->contentLength > 0 ? downloaded->contentLength : body.size();

    // Build the path to the requested file
    std::string path = path_cat( doc_root, req.target() );
    if ( req.target().back() == '/' ) path.append( "index.html" );

    // Respond to HEAD request
    if ( req.method() == http::verb::head )
    {
      http::response<http::empty_body> res{ http::status::ok, req.version() };
      res.set( http::field::server, BOOST_BEAST_VERSION_STRING );

      if ( downloaded->contentType.empty() ) res.set( http::field::content_type, mime_type( path ) );
      else res.set( http::field::content_type, downloaded->contentType );

      if ( !downloaded->etag.empty() ) res.set( http::field::etag, downloaded->etag );
      res.set( http::field::expires, downloaded->expirationTime() );
      res.set( http::field::last_modified, downloaded->lastModifiedTime() );
      if ( !downloaded->cacheControl.empty() ) res.set( http::field::cache_control, downloaded->cacheControl );

      res.content_length( size );
      res.keep_alive( req.keep_alive() );
      return send( std::move( res ) );
    }

    auto ifmatch = req[http::field::if_none_match];
    if ( beast::string_view{ downloaded->etag } == ifmatch )
    {
      return send( not_modified() );
    }

    auto ifmodified = req[http::field::if_modified_since];
    if ( beast::string_view{ downloaded->lastModifiedTime() } == ifmodified )
    {
      return send( not_modified() );
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple( std::move( body )),
        std::make_tuple( http::status::ok, req.version()) };
    res.set( http::field::server, BOOST_BEAST_VERSION_STRING );

    if ( downloaded->contentType.empty() ) res.set( http::field::content_type, mime_type( path ) );
    else res.set( http::field::content_type, downloaded->contentType );

    if ( !downloaded->etag.empty() ) res.set( http::field::etag, downloaded->etag );
    res.set( http::field::expires, downloaded->expirationTime() );
    res.set( http::field::last_modified, downloaded->lastModifiedTime() );
    if ( !downloaded->cacheControl.empty() ) res.set( http::field::cache_control, downloaded->cacheControl );

    res.content_length( size );
    res.keep_alive( req.keep_alive() );
    return send( std::move( res ) );
  }

//------------------------------------------------------------------------------

  // Report a failure
  void fail( beast::error_code ec, char const* what );

  // Handles an HTTP server connection
  class session
      : public boost::asio::coroutine,
          public std::enable_shared_from_this<session>
  {
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
      session& self_;
      std::shared_ptr<void> res_;

      explicit send_lambda( session& self ) : self_( self )
      {}

      template<bool isRequest, class Body, class Fields>
      void operator()( http::message<isRequest, Body, Fields>&& msg ) const
      {
        // The lifetime of the message has to extend
        // for the duration of the async operation so
        // we use a shared_ptr to manage it.
        auto sp = std::make_shared<
            http::message<isRequest, Body, Fields>>( std::move( msg ));

        // Store a type-erased version of the shared
        // pointer in the class to keep it alive.
        self_.res_ = sp;

        // Write the response
        http::async_write(
            self_.stream_,
            *sp,
            beast::bind_front_handler(
                &session::loop,
                self_.shared_from_this(),
                sp->need_eof()));
      }
    };

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;

  public:
    // Take ownership of the socket
    explicit session(
        tcp::socket&& socket,
        std::shared_ptr<std::string const> const& doc_root )
        : stream_( std::move( socket )), doc_root_( doc_root ),
        lambda_( *this )
    {
    }

    // Start the asynchronous operation
    void run()
    {
      // We need to be executing within a strand to perform async operations
      // on the I/O objects in this session. Although not strictly necessary
      // for single-threaded contexts, this example code is written to be
      // thread-safe by default.
      net::dispatch( stream_.get_executor(),
          beast::bind_front_handler( &session::loop,
              shared_from_this(),
              false,
              beast::error_code{},
              0 ));
    }

#include <boost/asio/yield.hpp>

    void loop( bool close, beast::error_code ec,
        std::size_t bytes_transferred )
    {
      boost::ignore_unused( bytes_transferred );
      reenter( *this )
      {
        for ( ;; )
        {
          // Make the request empty before reading,
          // otherwise the operation behavior is undefined.
          req_ = {};

          // Set the timeout.
          stream_.expires_after( std::chrono::seconds( 30 ));

          // Read a request
          yield http::async_read( stream_, buffer_, req_,
              beast::bind_front_handler(
                  &session::loop,
                  shared_from_this(),
                  false ));
          if ( ec == http::error::end_of_stream )
          {
            // The remote host closed the connection
            break;
          }
          if ( ec ) return fail( ec, "read" );

          // Send the response
          yield handle_request( *doc_root_, std::move( req_ ), lambda_,
              stream_.socket().remote_endpoint().address() );
          if ( ec ) return fail( ec, "write" );
          if ( close )
          {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
          }

          // We're done with the response so delete it
          res_ = nullptr;
        }

        // Send a TCP shutdown
        stream_.socket().shutdown( tcp::socket::shutdown_send, ec );

        // At this point the connection is closed gracefully
      }
    }

#include <boost/asio/unyield.hpp>
  };

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
  class listener
      : public boost::asio::coroutine,
          public std::enable_shared_from_this<listener>
  {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::shared_ptr<std::string const> doc_root_;

  public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        std::shared_ptr<std::string const> const& doc_root )
        : ioc_( ioc ), acceptor_( net::make_strand( ioc )),
        socket_( net::make_strand( ioc )), doc_root_( doc_root )
    {
      beast::error_code ec;

      // Open the acceptor
      acceptor_.open( endpoint.protocol(), ec );
      if ( ec )
      {
        fail( ec, "open" );
        return;
      }

      // Allow address reuse
      acceptor_.set_option( net::socket_base::reuse_address( true ), ec );
      if ( ec )
      {
        fail( ec, "set_option" );
        return;
      }

      // Bind to the server address
      acceptor_.bind( endpoint, ec );
      if ( ec )
      {
        fail( ec, "bind" );
        return;
      }

      // Start listening for connections
      acceptor_.listen( net::socket_base::max_listen_connections, ec );
      if ( ec )
      {
        fail( ec, "listen" );
        return;
      }
    }

    // Start accepting incoming connections
    void run()
    {
      loop();
    }

  private:

#include <boost/asio/yield.hpp>

    void loop( beast::error_code ec = {} )
    {
      reenter( *this )
      {
        for ( ;; )
        {
          yield
          acceptor_.async_accept(
              socket_,
              beast::bind_front_handler(
                  &listener::loop,
                  shared_from_this()));
          if ( ec )
          {
            fail( ec, "accept" );
          }
          else
          {
            // Create the session and run it
            std::make_shared<session>(
                std::move( socket_ ),
                doc_root_ )->run();
          }

          // Make sure each session gets its own strand
          socket_ = tcp::socket( net::make_strand( ioc_ ));
        }
      }
    }

#include <boost/asio/unyield.hpp>
  };
}
