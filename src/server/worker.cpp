//
// Created by Rakesh on 05/05/2020.
//

#include "worker.h"
#include "log/NanoLog.h"

using spt::server::Worker;

beast::string_view spt::server::mime_type(beast::string_view path)
{
  using beast::iequals;
  auto const ext = [&path]
  {
    auto const pos = path.rfind(".");
    if(pos == beast::string_view::npos)
      return beast::string_view{};
    return path.substr(pos);
  }();
  if(iequals(ext, ".htm") )  return "text/html";
  if(iequals(ext, ".html") ) return "text/html";
  if(iequals(ext, ".php") )  return "text/html";
  if(iequals(ext, ".css") )  return "text/css";
  if(iequals(ext, ".txt") )  return "text/plain";
  if(iequals(ext, ".js") )   return "application/javascript";
  if(iequals(ext, ".json") ) return "application/json";
  if(iequals(ext, ".xml") )  return "application/xml";
  if(iequals(ext, ".swf") )  return "application/x-shockwave-flash";
  if(iequals(ext, ".flv") )  return "video/x-flv";
  if(iequals(ext, ".png") )  return "image/png";
  if(iequals(ext, ".jpe") )  return "image/jpeg";
  if(iequals(ext, ".jpeg") ) return "image/jpeg";
  if(iequals(ext, ".jpg") )  return "image/jpeg";
  if(iequals(ext, ".gif") )  return "image/gif";
  if(iequals(ext, ".bmp") )  return "image/bmp";
  if(iequals(ext, ".ico") )  return "image/vnd.microsoft.icon";
  if(iequals(ext, ".tiff") ) return "image/tiff";
  if(iequals(ext, ".tif") )  return "image/tiff";
  if(iequals(ext, ".svg") )  return "image/svg+xml";
  if(iequals(ext, ".svgz") ) return "image/svg+xml";
  return "application/text";
}

void Worker::start()
{
  accept();
  check_deadline();
}

void Worker::accept()
{
  // Clean up any previous connection.
  beast::error_code ec;
  socket_.close( ec );
  buffer_.consume( buffer_.size() );

  acceptor_.async_accept(
      socket_,
      [this]( beast::error_code ec )
      {
        if ( ec )
        {
          accept();
        }
        else
        {
          // Request must be fully processed within 60 seconds.
          request_deadline_.expires_after( std::chrono::seconds( 60 ) );
          read_request();
        }
      } );
}

void Worker::read_request()
{
  // On each read the parser needs to be destroyed and
  // recreated. We store it in a boost::optional to
  // achieve that.
  //
  // Arguments passed to the parser constructor are
  // forwarded to the message object. A single argument
  // is forwarded to the body constructor.
  //
  // We construct the dynamic body with a 1MB limit
  // to prevent vulnerability to buffer attacks.
  //
  parser_.emplace( std::piecewise_construct,
      std::make_tuple(), std::make_tuple( alloc_ ) );

  http::async_read(
      socket_, buffer_, *parser_,
      [this]( beast::error_code ec, std::size_t )
      {
        if ( ec ) accept();
        else process_request( parser_->get() );
      } );
}

void Worker::process_request(
    http::request <request_body_t, http::basic_fields<alloc_t>> const& req )
{
  switch ( req.method() )
  {
  case http::verb::get:
    send_file( req.target() );
    break;
  default:
    // We return responses indicating an error if
    // we do not recognize the request method.
    send_bad_response(
        http::status::bad_request,
        "Invalid request-method '" + std::string( req.method_string() ) +
            "'\r\n" );
    break;
  }
}

void Worker::send_bad_response( http::status status, std::string const& error )
{
  string_response_.emplace(
      std::piecewise_construct,
      std::make_tuple(), std::make_tuple( alloc_ ) );

  string_response_->result( status );
  string_response_->keep_alive( false );
  string_response_->set( http::field::server, "s3 Proxy" );
  string_response_->set( http::field::content_type, "text/plain" );
  string_response_->body() = error;
  string_response_->prepare_payload();

  string_serializer_.emplace( *string_response_ );

  http::async_write(
      socket_, *string_serializer_,
      [this]( beast::error_code ec, std::size_t )
      {
        socket_.shutdown( tcp::socket::shutdown_send, ec );
        string_serializer_.reset();
        string_response_.reset();
        accept();
      });
}

void Worker::send_file( beast::string_view target )
{
  // Request path must be absolute and not contain "..".
  if ( target.empty() || target[0] != '/' || target.find("..") != std::string::npos )
  {
    send_bad_response( http::status::not_found, "File not found\r\n" );
    return;
  }

  std::string full_path = doc_root_;
  if ( target == "/" ) full_path.append( "/index.html" );
  else full_path.append( target.data(), target.size() );

  LOG_DEBUG << "Serving path: " << full_path;

  http::file_body::value_type file;
  beast::error_code ec;
  file.open( full_path.c_str(), beast::file_mode::read, ec );
  if( ec )
  {
    send_bad_response(  http::status::not_found, "File not found\r\n" );
    return;
  }

  file_response_.emplace(
      std::piecewise_construct,
      std::make_tuple(),
      std::make_tuple(alloc_) );

  file_response_->result( http::status::ok );
  file_response_->keep_alive( false );
  file_response_->set( http::field::server, "s3 Proxy" );
  file_response_->set( http::field::content_type, mime_type( std::string( target ) ) );
  file_response_->body() = std::move( file );
  file_response_->prepare_payload();

  file_serializer_.emplace( *file_response_ );

  http::async_write(
      socket_,
      *file_serializer_,
      [this](beast::error_code ec, std::size_t)
      {
        socket_.shutdown( tcp::socket::shutdown_send, ec );
        file_serializer_.reset();
        file_response_.reset();
        accept();
      });
}

void Worker::check_deadline()
{
  // The deadline may have moved, so check it has really passed.
  if ( request_deadline_.expiry() <= std::chrono::steady_clock::now() )
  {
    // Close socket to cancel any outstanding operation.
    socket_.close();

    // Sleep indefinitely until we're given a new deadline.
    request_deadline_.expires_at( std::chrono::steady_clock::time_point::max() );
  }

  request_deadline_.async_wait( [this](beast::error_code) { check_deadline(); } );
}
