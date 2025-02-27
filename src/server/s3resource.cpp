//
// Created by Rakesh on 19/12/2020.
//

#include "handlers.hpp"
#include "log/NanoLog.hpp"
#include "model/config.hpp"
#include "queue/queuemanager.hpp"
#include "server/s3util.hpp"
#include "util/compress.hpp"

#include <array>
#include <filesystem>
#include <fstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/core.hpp>

namespace
{
  namespace impl
  {
    namespace beast = boost::beast;         // from <boost/beast.hpp>

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
      if ( iequals( ext, ".yml" )) return "application/yaml";
      if ( iequals( ext, ".yaml" )) return "application/yaml";
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
      if ( base.empty() ) return std::string( path );
      std::string result( base );
  #ifdef BOOST_MSVC
      char constexpr path_separator = '\\';
      if( result.back() == path_separator ) result.resize(result.size() - 1);
      result.append( path.data(), path.size() );
      for( auto& c : result ) if ( c == '/' ) c = path_separator;
  #else
      char constexpr path_separator = '/';
      if ( result.back() == path_separator ) result.resize( result.size() - 1 );
      result.append( path.data(), path.size() );
  #endif
      return result;
    }

    bool should_compress( beast::string_view path )
    {
      using beast::iequals;
      auto const ext = [&path]
      {
        auto const pos = path.rfind( "." );
        if ( pos == beast::string_view::npos )
          return beast::string_view{};
        return path.substr( pos );
      }();
      if ( iequals( ext, ".htm" )) return true;
      if ( iequals( ext, ".html" )) return true;
      if ( iequals( ext, ".css" )) return true;
      if ( iequals( ext, ".xss" )) return true;
      if ( iequals( ext, ".txt" )) return true;
      if ( iequals( ext, ".js" )) return true;
      if ( iequals( ext, ".json" )) return true;
      if ( iequals( ext, ".xml" )) return true;
      if ( iequals( ext, ".docx" )) return true;
      if ( iequals( ext, ".xlsx" )) return true;
      if ( iequals( ext, ".pptx" )) return true;
      return false;
    }

  }
}

spt::server::Response spt::server::get( const http2::framework::RoutingRequest& rr, const boost::container::flat_map<std::string_view, std::string_view>& )
{
  using std::operator ""s;
  static const auto methods = std::array{ "GET"s, "HEAD"s, "OPTIONS"s };
  const auto& conf = model::Configuration::instance();
  auto resp = Response( rr.req.header );

  try
  {
    resp.path = rr.req.path;
    if ( resp.path.empty() ) resp.path = "/";

    if ( const auto pos = resp.path.find_first_of( '?' ); pos != std::string::npos )
    {
      resp.path = resp.path.substr( 0, pos );
    }

    // Request path must be absolute and not contain "..".
    if ( resp.path.empty() || resp.path[0] != '/' || resp.path.contains( ".." ) )
    {
      resp.status = 400;
      resp.body = "Illegal request-target";
      resp.contentLength = resp.body.size();
      resp.set( methods, Response::origins() );
      return resp;
    }

    // Query strings make no sense in the context of trying to
    // serve files stored in S3 buckets.  If configured as error reject.
    if ( conf.rejectQueryStrings && resp.path.contains( '?' ) )
    {
      LOG_DEBUG << "Request " << resp.path << " rejected due to query string policy";
      resp.status = 400;
      resp.body = "Illegal request-target";
      resp.contentLength = resp.body.size();
      resp.set( methods, Response::origins() );
      return resp;
    }

    if ( resp.path.back() == '/' )
    {
      resp.path.append( "index.html" );
    }

    auto downloaded = S3Util::instance().get( resp.path );
    if ( ! downloaded || downloaded->fileName.empty() )
    {
      LOG_WARN << "Error downloading resource " << resp.path;
      resp.status = 404;
      resp.body = "Not found";
      resp.contentLength = resp.body.size();
      resp.set( methods, Response::origins() );
      return resp;
    }

    LOG_INFO << "Downloaded resource " << resp.path << " from S3\n" << downloaded->str();

    // Build the path to the requested file
    std::string path = impl::path_cat( conf.cacheDir, resp.path );
    if ( resp.path.back() == '/' ) path.append( "index.html" );

    if ( downloaded->contentType.empty() ) resp.mimeType = std::string{ impl::mime_type( path ) };
    else resp.mimeType = downloaded->contentType;

    const auto compressed = shouldCompress( rr.req ) && impl::should_compress( path );
    LOG_DEBUG << "Compressed request for " << resp.path;

    const auto fn = compressed ? util::compressedFileName( downloaded->fileName ) : downloaded->fileName;
    std::error_code ec;
    if ( compressed && !std::filesystem::exists( fn, ec ) )
    {
      if ( !ec ) util::compress( downloaded->fileName, fn );
    }

    const auto size = std::filesystem::file_size( fn, ec );
    if ( ec )
    {
      LOG_CRIT << "I/O error. " << ec.message();
      resp.status = 500;
      resp.body = "I/O error";
      resp.contentLength = resp.body.size();
      resp.set( methods, Response::origins() );
      return resp;
    }

    // Handle the case where the file doesn't exist
    if ( !std::filesystem::exists( fn, ec ) )
    {
      resp.status = 404;
      resp.body = "Not found";
      resp.contentLength = resp.body.size();
      resp.set( methods, Response::origins() );
      return resp;
    }

    resp.contentLength = size;
    resp.compressed = compressed;

    const auto additionalHeaders = [&resp, &downloaded]()
    {
      resp.headers.emplace( "content-type", nghttp2::asio_http2::header_value{ resp.mimeType, false } );
      resp.headers.emplace( "etag", nghttp2::asio_http2::header_value{ downloaded->etag, false } );
      resp.headers.emplace( "expires", nghttp2::asio_http2::header_value{ downloaded->expirationTime(), false } );
      resp.headers.emplace( "last-modified", nghttp2::asio_http2::header_value{ downloaded->lastModifiedTime(), false } );

      if ( !downloaded->cacheControl.empty() )
      {
        resp.headers.emplace( "cache-control", nghttp2::asio_http2::header_value{ downloaded->cacheControl, false } );
      }
    };

    // Respond to HEAD request
    if ( rr.req.method == "HEAD" )
    {
      resp.contentLength = downloaded->contentLength;
      resp.headers.emplace( "content-length", nghttp2::asio_http2::header_value{ std::to_string( downloaded->contentLength ), false } );
      resp.status = 200;
      resp.set( methods, Response::origins() );
      additionalHeaders();
      return resp;
    }

    if ( auto ifmatch = ifNoneMatch( rr.req ); downloaded->etag == ifmatch )
    {
      resp.contentLength = 0;
      resp.status = 304;
      resp.set( methods, Response::origins() );
      return resp;
    }

    if ( auto ifmodified = ifModifiedSince( rr.req ); downloaded->lastModifiedTime() == ifmodified )
    {
      resp.contentLength = 0;
      resp.status = 304;
      resp.set( methods, Response::origins() );
      return resp;
    }

    auto stream = std::ifstream( fn, std::ios::in | std::ios::binary );
    stream.exceptions( std::ios_base::badbit );
    if ( !stream )
    {
      LOG_WARN << "File at " << fn << " does not exist";
      resp.status = 404;
      resp.body = "Not found";
      resp.contentLength = resp.body.size();
      resp.set( methods, Response::origins() );
      return resp;
    }

    resp.body.resize( resp.contentLength, '\0' );
    stream.read( &resp.body[0], static_cast<std::streamsize>( resp.contentLength ) );

    resp.status = 200;
    resp.set( methods, Response::origins() );
    additionalHeaders();
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error processing request " << ex.what();
    resp.status = 500;
    resp.body = "Unknown error";
    resp.contentLength = resp.body.size();
    resp.set( methods, Response::origins() );
  }

  return resp;
}

spt::server::Response spt::server::clear( const http2::framework::RoutingRequest& rr, const boost::container::flat_map<std::string_view, std::string_view>& )
{
  using std::operator ""s;
  static const auto methods = std::array{ "DELETE"s, "OPTIONS"s };
  auto resp = Response( rr.req.header );

  try
  {
    resp.path = rr.req.path;
    if ( resp.path.empty() ) resp.path = "/";

    if ( const auto pos = resp.path.find_first_of( '?' ); pos != std::string::npos )
    {
      resp.path = resp.path.substr( 0, pos );
    }

    auto bearer = authorise( rr.req );
    if ( bearer.empty() )
    {
      resp.status = 403;
      resp.body = "Forbidden";
      resp.contentLength = resp.body.size();
      resp.set( methods, Response::origins() );
      return resp;
    }

    const auto prefix = std::string("Bearer ");
    if ( !boost::algorithm::starts_with( bearer, prefix ) )
    {
      resp.status = 400;
      resp.body = "Invalid Authorization header";
      resp.contentLength = resp.body.size();
      resp.set( methods, Response::origins() );
      return resp;
    }

    std::string temp{ bearer };
    temp.erase( 0, prefix.size() );

    if ( S3Util::instance().clear( temp ) )
    {
      resp.status = 304;
      resp.set( methods, Response::origins() );
      return resp;
    }

    resp.status = 403;
    resp.body = "Forbidden";
    resp.contentLength = resp.body.size();
    resp.set( methods, Response::origins() );
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error processing request " << ex.what();
    resp.status = 500;
    resp.body = "Unknown error";
    resp.contentLength = resp.body.size();
    resp.set( methods, Response::origins() );
  }

  return resp;
}
