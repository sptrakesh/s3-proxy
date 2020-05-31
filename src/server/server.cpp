//
// Created by Rakesh on 05/05/2020.
//

#include "server.h"
#include "s3util.h"
#include "client/mmdb.h"
#include "log/NanoLog.h"
#include "queue/poller.h"
#include "queue/queuemanager.h"

#include <iostream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace spt::server::impl
{
  beast::string_view mime_type( beast::string_view path )
  {
    using beast::iequals;
    auto const ext = [&path]
    {
      auto const pos = path.rfind( "." );
      if ( pos == beast::string_view::npos )
        return beast::string_view{};
      return path.substr( pos );
    }();
    if ( iequals( ext, ".htm" )) return "text/html";
    if ( iequals( ext, ".html" )) return "text/html";
    if ( iequals( ext, ".php" )) return "text/html";
    if ( iequals( ext, ".css" )) return "text/css";
    if ( iequals( ext, ".txt" )) return "text/plain";
    if ( iequals( ext, ".js" )) return "application/javascript";
    if ( iequals( ext, ".json" )) return "application/json";
    if ( iequals( ext, ".xml" )) return "application/xml";
    if ( iequals( ext, ".swf" )) return "application/x-shockwave-flash";
    if ( iequals( ext, ".flv" )) return "video/x-flv";
    if ( iequals( ext, ".png" )) return "image/png";
    if ( iequals( ext, ".jpe" )) return "image/jpeg";
    if ( iequals( ext, ".jpeg" )) return "image/jpeg";
    if ( iequals( ext, ".jpg" )) return "image/jpeg";
    if ( iequals( ext, ".gif" )) return "image/gif";
    if ( iequals( ext, ".bmp" )) return "image/bmp";
    if ( iequals( ext, ".ico" )) return "image/vnd.microsoft.icon";
    if ( iequals( ext, ".tiff" )) return "image/tiff";
    if ( iequals( ext, ".tif" )) return "image/tiff";
    if ( iequals( ext, ".svg" )) return "image/svg+xml";
    if ( iequals( ext, ".svgz" )) return "image/svg+xml";
    if ( iequals( ext, ".docx" ))
      return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if ( iequals( ext, ".xlsx" ))
      return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if ( iequals( ext, ".pptx" ))
      return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    if ( iequals( ext, ".kar" )) return "audio/midi";
    if ( iequals( ext, ".midi" )) return "audio/midi";
    if ( iequals( ext, ".mid" )) return "audio/midi";
    if ( iequals( ext, ".mp3" )) return "audio/mpeg";
    if ( iequals( ext, ".ogg" )) return "audio/ogg";
    if ( iequals( ext, ".m4a" )) return "audio/x-m4a";
    if ( iequals( ext, ".ra" )) return "audio/x-realaudio";
    if ( iequals( ext, ".3gp" )) return "video/3gpp";
    if ( iequals( ext, ".3gpp" )) return "video/3gpp";
    if ( iequals( ext, ".ts" )) return "video/mp2t";
    if ( iequals( ext, ".mp4" )) return "video/mp4";
    if ( iequals( ext, ".mpg" )) return "video/mpeg";
    if ( iequals( ext, ".mpeg" )) return "video/mpeg";
    if ( iequals( ext, ".mov" )) return "video/quicktime";
    if ( iequals( ext, "webm" )) return "video/webm";
    if ( iequals( ext, "flv" )) return "video/x-flv";
    if ( iequals( ext, "m4v" )) return "video/x-m4v";
    if ( iequals( ext, "mng" )) return "video/x-mng";
    if ( iequals( ext, "asf" )) return "video/x-ms-asf";
    if ( iequals( ext, "asx" )) return "video/x-ms-asf";
    if ( iequals( ext, "wmv" )) return "video/x-ms-wmv";
    if ( iequals( ext, "avi" )) return "video/x-msvideo";
    return "application/text";
  }

  std::string path_cat( beast::string_view base, beast::string_view path )
  {
    if ( base.empty())
      return std::string( path );
    std::string result( base );
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if ( result.back() == path_separator )
      result.resize( result.size() - 1 );
    result.append( path.data(), path.size());
#endif
    return result;
  }

  void fail( beast::error_code ec, char const* what )
  {
    LOG_WARN << what << ": " << ec.message();
  }

  // This function produces an HTTP response for the given
  // request. The type of the response object depends on the
  // contents of the request, so the interface requires the
  // caller to pass a generic lambda for receiving the response.
  template<
      class Body, class Allocator,
      class Send>
  void
  handle_request( const model::Configuration* config,
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
          res.set( http::field::content_type, "text/plain" );
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
          res.set( http::field::content_type, "text/plain" );
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
          res.set( http::field::content_type, "text/plain" );
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
          res.set( http::field::content_type, "text/plain" );
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

    auto metric = model::Metric{};
    const auto method = http::to_string( req.method() );
    metric.method = std::string{ method.data(), method.size() };
    metric.resource = std::string{ req.target().data(), req.target().size() };

    auto ip = req["x-real-ip"];
    if ( ip.empty() ) ip = req["x-forwarded-for"];
    if ( ip.empty() ) ip = client.to_string();
    metric.ipaddress = std::string{ ip.data(), ip.size() };
    LOG_DEBUG << "Request from " << metric.ipaddress;

    const auto ch = req[http::field::accept_encoding];
    const auto compressed = ( boost::algorithm::contains( ch, "gzip" ) );
    LOG_DEBUG << "Compressed request " << ch.data() << " : " << compressed;

    // Make sure we can handle the method
    if ( req.method() != http::verb::get &&
        req.method() != http::verb::head &&
        req.method() != http::verb::options )
    {
      metric.status = 405;
      queue::QueueManager::instance().publish( std::move( metric ) );
      return send( not_allowed() );
    }

    if ( req.method() == http::verb::options )
    {
      http::response<http::empty_body> res{ http::status::no_content,
          req.version() };
      res.set( "Access-Control-Allow-Origin", "*" );
      res.set( "Access-Control-Allow-Methods", "GET" );
      res.set( http::field::server, BOOST_BEAST_VERSION_STRING );
      res.keep_alive( req.keep_alive() );
      metric.status = 200;
      queue::QueueManager::instance().publish( std::move( metric ) );
      return send( std::move( res ) );
    }

    if ( http::verb::get == req.method() && req.target() == "/_proxy/_private/_cache/clear" )
    {
      auto bearer = req["authorization"];
      if ( bearer.empty() ) bearer = req["Authorization"];
      if ( bearer.empty() )
      {
        metric.status = 403;
        queue::QueueManager::instance().publish( std::move( metric ) );
        return send( forbidden() );
      }

      const auto prefix = std::string("Bearer ");
      if ( !boost::algorithm::starts_with( bearer, prefix ) )
      {
        metric.status = 400;
        queue::QueueManager::instance().publish( std::move( metric ) );
        return send( bad_request( "Invalid Authorization header" ) );
      }

      std::string temp{ bearer };
      temp.erase( 0, prefix.size() );

      if ( S3Util::instance().clear( temp ) )
      {
        metric.status = 304;
        queue::QueueManager::instance().publish( std::move( metric ) );
        return send( not_modified() );
      }
      else
      {
        metric.status = 403;
        queue::QueueManager::instance().publish( std::move( metric ) );
        return send( forbidden() );
      }
    }

    // Request path must be absolute and not contain "..".
    if ( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find( ".." ) != beast::string_view::npos )
    {
      metric.status = 400;
      queue::QueueManager::instance().publish( std::move( metric ) );
      return send( bad_request( "Illegal request-target" ));
    }

    if ( req.target().back() == '/' )
    {
      metric.resource.append( "index.html" );
    }

    auto downloaded = S3Util::instance().get( metric.resource );
    if ( ! downloaded || downloaded->fileName.empty() )
    {
      LOG_WARN << "Error downloading resource " << metric.resource;
      metric.status = 404;
      queue::QueueManager::instance().publish( std::move( metric ) );
      return send( not_found( req.target() ) );
    }
    else
    {
      LOG_INFO << "Downloaded resource " << metric.resource << " from S3\n" << downloaded->str();
    }

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    if ( compressed )
    {
      body.open( downloaded->fileNameCompressed.c_str(), beast::file_mode::scan, ec );
    }
    else
    {
      body.open( downloaded->fileName.c_str(), beast::file_mode::scan, ec );
    }

    // Handle the case where the file doesn't exist
    if ( ec == beast::errc::no_such_file_or_directory ) return send( not_found( req.target() ) );

    // Handle an unknown error
    if ( ec )
    {
      metric.status = 500;
      queue::QueueManager::instance().publish( std::move( metric ) );
      return send( server_error( ec.message() ) );
    }

    metric.size = body.size();

    // Build the path to the requested file
    std::string path = path_cat( config->cacheDir, req.target() );
    if ( req.target().back() == '/' ) path.append( "index.html" );

    // Respond to HEAD request
    if ( req.method() == http::verb::head )
    {
      http::response<http::empty_body> res{ http::status::ok, req.version() };
      res.set( http::field::server, BOOST_BEAST_VERSION_STRING );

      if ( downloaded->contentType.empty() ) metric.mimeType = mime_type( path ).to_string();
      else metric.mimeType = downloaded->contentType;
      res.set( http::field::content_type, metric.mimeType );

      if ( !downloaded->etag.empty() ) res.set( http::field::etag, downloaded->etag );
      res.set( http::field::expires, downloaded->expirationTime() );
      res.set( http::field::last_modified, downloaded->lastModifiedTime() );
      if ( !downloaded->cacheControl.empty() ) res.set( http::field::cache_control, downloaded->cacheControl );

      res.content_length( downloaded->contentLength );
      res.keep_alive( req.keep_alive() );
      metric.status = 200;
      queue::QueueManager::instance().publish( std::move( metric ) );
      return send( std::move( res ) );
    }

    auto ifmatch = req[http::field::if_none_match];
    if ( beast::string_view{ downloaded->etag } == ifmatch )
    {
      metric.status = 304;
      queue::QueueManager::instance().publish( std::move( metric ) );
      return send( not_modified() );
    }

    auto ifmodified = req[http::field::if_modified_since];
    if ( beast::string_view{ downloaded->lastModifiedTime() } == ifmodified )
    {
      metric.status = 304;
      queue::QueueManager::instance().publish( std::move( metric ) );
      return send( not_modified() );
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple( std::move( body )),
        std::make_tuple( http::status::ok, req.version()) };
    res.set( http::field::server, BOOST_BEAST_VERSION_STRING );

    if ( compressed ) res.set( http::field::content_encoding, "gzip" );
    if ( downloaded->contentType.empty() ) res.set( http::field::content_type, mime_type( path ) );
    else res.set( http::field::content_type, downloaded->contentType );

    if ( !downloaded->etag.empty() ) res.set( http::field::etag, downloaded->etag );
    res.set( http::field::expires, downloaded->expirationTime() );
    res.set( http::field::last_modified, downloaded->lastModifiedTime() );
    if ( !downloaded->cacheControl.empty() ) res.set( http::field::cache_control, downloaded->cacheControl );

    res.content_length( metric.size );
    res.keep_alive( req.keep_alive() );
    metric.status = 200;
    queue::QueueManager::instance().publish( std::move( metric ) );
    return send( std::move( res ) );
  }

  class session : public std::enable_shared_from_this<session>
  {
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
      session& self_;

      explicit
      send_lambda(session& self)
          : self_(self)
      {
      }

      template<bool isRequest, class Body, class Fields>
      void
      operator()(http::message<isRequest, Body, Fields>&& msg) const
      {
        // The lifetime of the message has to extend
        // for the duration of the async operation so
        // we use a shared_ptr to manage it.
        auto sp = std::make_shared<
            http::message<isRequest, Body, Fields>>(std::move(msg));

        // Store a type-erased version of the shared
        // pointer in the class to keep it alive.
        self_.res_ = sp;

        // Write the response
        http::async_write(
            self_.stream_,
            *sp,
            beast::bind_front_handler(
                &session::on_write,
                self_.shared_from_this(),
                sp->need_eof()));
      }
    };

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    model::Configuration* configuration;
    send_lambda lambda_;

  public:
    // Take ownership of the stream
    session(tcp::socket&& socket, model::Configuration* configuration)
        : stream_(std::move(socket))
        , configuration(configuration)
        , lambda_(*this)
    {
    }

    // Start the asynchronous operation
    void
    run()
    {
      // We need to be executing within a strand to perform async operations
      // on the I/O objects in this session. Although not strictly necessary
      // for single-threaded contexts, this example code is written to be
      // thread-safe by default.
      net::dispatch(stream_.get_executor(),
          beast::bind_front_handler(
              &session::do_read,
              shared_from_this()));
    }

    void
    do_read()
    {
      // Make the request empty before reading,
      // otherwise the operation behavior is undefined.
      req_ = {};

      // Set the timeout.
      stream_.expires_after(std::chrono::seconds(30));

      // Read a request
      http::async_read(stream_, buffer_, req_,
          beast::bind_front_handler(
              &session::on_read,
              shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
      boost::ignore_unused(bytes_transferred);

      // This means they closed the connection
      if(ec == http::error::end_of_stream)
        return do_close();

      if(ec)
        return fail(ec, "read");

      // Send the response
      handle_request(configuration, std::move(req_), lambda_, stream_.socket().remote_endpoint().address());
    }

    void
    on_write(
        bool close,
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
      boost::ignore_unused(bytes_transferred);

      if(ec)
        return fail(ec, "write");

      if(close)
      {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return do_close();
      }

      // We're done with the response so delete it
      res_ = nullptr;

      // Read another request
      do_read();
    }

    void
    do_close()
    {
      // Send a TCP shutdown
      beast::error_code ec;
      stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

      // At this point the connection is closed gracefully
    }
  };

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
  class listener : public std::enable_shared_from_this<listener>
  {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    model::Configuration* configuration;

  public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        model::Configuration* configuration)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , configuration(configuration)
    {
      beast::error_code ec;

      // Open the acceptor
      acceptor_.open(endpoint.protocol(), ec);
      if(ec)
      {
        fail(ec, "open");
        return;
      }

      // Allow address reuse
      acceptor_.set_option(net::socket_base::reuse_address(true), ec);
      if(ec)
      {
        fail(ec, "set_option");
        return;
      }

      // Bind to the server address
      acceptor_.bind(endpoint, ec);
      if(ec)
      {
        fail(ec, "bind");
        return;
      }

      // Start listening for connections
      acceptor_.listen(
          net::socket_base::max_listen_connections, ec);
      if(ec)
      {
        fail(ec, "listen");
        return;
      }
    }

    // Start accepting incoming connections
    void
    run()
    {
      do_accept();
    }

  private:
    void
    do_accept()
    {
      // The new connection gets its own strand
      acceptor_.async_accept(
          net::make_strand(ioc_),
          beast::bind_front_handler(
              &listener::on_accept,
              shared_from_this()));
    }

    void
    on_accept(beast::error_code ec, tcp::socket socket)
    {
      if(ec)
      {
        fail(ec, "accept");
      }
      else
      {
        // Create the session and run it
        std::make_shared<session>(
            std::move(socket),
            configuration)->run();
      }

      // Accept another connection
      do_accept();
    }
  };
}

int spt::server::run( model::Configuration::Ptr configuration )
{
  try
  {
    S3Util::instance( configuration );
    LOG_INFO << "Initialised AWS S3 Util";

    auto const address = net::ip::make_address( "0.0.0.0" );
    net::io_context ioc{ configuration->threads + 1 };

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&](beast::error_code const&, int)
        {
          ioc.stop();
        });

    queue::QueueManager::instance( configuration.get() );

    // Create and launch a listening port
    std::make_shared<impl::listener>( ioc,
        tcp::endpoint{ address, static_cast<unsigned short>( configuration->port ) },
        configuration.get() )->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve( configuration->threads );
    for ( auto i = configuration->threads - 1; i > 0; --i )
    {
      v.emplace_back( [&ioc] { ioc.run(); } );
    }

    auto poller = queue::Poller{ ioc, configuration };

    if ( model::publishMetrics( *configuration ) )
    {
      client::MMDBClient::instance( configuration );
      v.emplace_back( std::thread{ &spt::queue::Poller::run, &poller } );
    }

    LOG_INFO << "HTTP service started";
    ioc.run();

    LOG_INFO << "HTTP service stopped";
    poller.stop();
    for ( auto& t : v ) t.join();

    LOG_INFO << "All I/O threads stopped";
    return EXIT_SUCCESS;
  }
  catch ( const std::exception& e )
  {
    LOG_CRIT << "Error: " << e.what();
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
