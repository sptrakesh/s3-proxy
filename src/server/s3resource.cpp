//
// Created by Rakesh on 19/12/2020.
//

#include "handlers.h"
#include "log/NanoLog.h"
#include "model/config.h"
#include "queue/queuemanager.h"
#include "server/s3util.h"
#include "util/compress.h"

#include <filesystem>
#include <unordered_set>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/core.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>

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
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size() );
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if ( result.back() == path_separator )
      result.resize( result.size() - 1 );
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

void spt::server::handleRoot( const nghttp2::asio_http2::server::request& req,
    const nghttp2::asio_http2::server::response& res )
{
  auto static const methods = std::unordered_set<std::string>{ "GET", "HEAD", "OPTIONS" };
  const auto& conf = model::Configuration::instance();

  const auto st = std::chrono::steady_clock::now();
  auto metric = model::Metric{};
  try
  {
    metric.method = req.method();
    metric.resource = req.uri().path;

    const auto pos = metric.resource.find_first_of( '?' );
    if ( pos != std::string::npos )
    {
      metric.resource = metric.resource.substr( 0, pos );
    }

    metric.ipaddress = ipaddress( req );
    LOG_DEBUG << "Request from " << metric.ipaddress;

    if ( req.method() == "OPTIONS" )
    {
      metric.status = 200;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return cors( res );
    }

    if ( methods.find( req.method() ) == std::cend( methods ) )
    {
      metric.status = 405;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return write( 405, "Method Not Allowed", res );
    }

    if ( req.method() == "GET" && metric.resource == "/_proxy/_private/_cache/clear" )
    {
      auto bearer = authorise( req );
      if ( bearer.empty() )
      {
        metric.status = 403;
        const auto et = std::chrono::steady_clock::now();
        const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
        metric.time = delta.count();
        queue::QueueManager::instance().publish( std::move( metric ) );
        return write( 403, {}, res );
      }

      const auto prefix = std::string("Bearer ");
      if ( !boost::algorithm::starts_with( bearer, prefix ) )
      {
        metric.status = 400;
        const auto et = std::chrono::steady_clock::now();
        const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
        metric.time = delta.count();
        queue::QueueManager::instance().publish( std::move( metric ) );
        return write( 400, "Invalid Authorization header", res );
      }

      std::string temp{ bearer };
      temp.erase( 0, prefix.size() );

      if ( S3Util::instance().clear( temp ) )
      {
        metric.status = 304;
        const auto et = std::chrono::steady_clock::now();
        const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
        metric.time = delta.count();
        queue::QueueManager::instance().publish( std::move( metric ) );
        return write( 304, {}, res );
      }
      else
      {
        metric.status = 403;
        const auto et = std::chrono::steady_clock::now();
        const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
        metric.time = delta.count();
        queue::QueueManager::instance().publish( std::move( metric ) );
        return write( 403, {}, res );
      }
    }

    // Request path must be absolute and not contain "..".
    if ( metric.resource.empty() ||
        metric.resource[0] != '/' ||
        metric.resource.find( ".." ) != std::string::npos )
    {
      metric.status = 400;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return write( 400, "Illegal request-target", res );
    }

    // Query strings make no sense in the context of trying to
    // serve files stored in S3 buckets.  If configured as error reject.
    if ( conf.rejectQueryStrings && metric.resource.find( '?' ) != std::string::npos )
    {
      LOG_DEBUG << "Request " << metric.resource << " rejected due to query string policy";
      metric.status = 400;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return write( 400, "Illegal request-target", res );
    }

    if ( metric.resource.back() == '/' )
    {
      metric.resource.append( "index.html" );
    }

    auto downloaded = S3Util::instance().get( metric.resource );
    if ( ! downloaded || downloaded->fileName.empty() )
    {
      LOG_WARN << "Error downloading resource " << metric.resource;
      metric.status = 404;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return write( 404, {}, res );
    }
    else
    {
      LOG_INFO << "Downloaded resource " << metric.resource << " from S3\n" << downloaded->str();
    }

    // Build the path to the requested file
    std::string path = impl::path_cat( conf.cacheDir, metric.resource );
    if ( metric.resource.back() == '/' ) path.append( "index.html" );

    if ( downloaded->contentType.empty() ) metric.mimeType = std::string{ impl::mime_type( path ) };
    else metric.mimeType = downloaded->contentType;

    const auto compressed = shouldCompress( req ) && impl::should_compress( path );
    LOG_DEBUG << "Compressed request for " << metric.resource;

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
      metric.status = 500;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return write( 500, "I/O error", res );
    }

    // Handle the case where the file doesn't exist
    if ( !std::filesystem::exists( fn, ec ) )
    {
      metric.status = 404;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return write( 404, {}, res );
    }

    metric.size = size;
    metric.compressed = compressed;

    auto headers = nghttp2::asio_http2::header_map{
        {"Access-Control-Allow-Origin", {"*", false}},
        {"Access-Control-Allow-Methods", {"GET,OPTIONS", false}},
        {"Access-Control-Allow-Headers", {"*, authorization", false}},
        {"content-type", { metric.mimeType, false } },
        { "etag", { downloaded->etag, false } },
        { "expires", { downloaded->expirationTime(), false } },
        { "last-modified", { downloaded->lastModifiedTime(), false } }
    };

    if ( !downloaded->cacheControl.empty() )
    {
      headers.emplace( "cache-control",
          nghttp2::asio_http2::header_value{ downloaded->cacheControl, false } );
    }

    // Respond to HEAD request
    if ( req.method() == "HEAD" )
    {
      headers.emplace( "content-length",
          nghttp2::asio_http2::header_value{ std::to_string( downloaded->contentLength ), false } );
      metric.status = 200;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      res.write_head( 200, std::move( headers ) );
      return write( 200, {}, res );
    }

    auto ifmatch = ifNoneMatch( req );
    if ( downloaded->etag == ifmatch )
    {
      metric.status = 304;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return write( 304, {}, res );
    }

    auto ifmodified = ifModifiedSince( req );
    if ( downloaded->lastModifiedTime() == ifmodified )
    {
      metric.status = 304;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return write( 304, {}, res );
    }

    if ( compressed )
    {
      headers.emplace( "content-encoding", nghttp2::asio_http2::header_value{ "gzip", false } );
    }
    headers.emplace( "content-length",
        nghttp2::asio_http2::header_value{ std::to_string( metric.size ), false } );

    metric.status = 200;
    const auto et = std::chrono::steady_clock::now();
    const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
    metric.time = delta.count();
    queue::QueueManager::instance().publish( std::move( metric ) );

    res.write_head( 200, std::move( headers ) );
    return res.end( nghttp2::asio_http2::file_generator( fn ) );
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error processing request " << ex.what();
    return write( 500, "Unknown error",  res );
  }
}

