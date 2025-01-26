//
// Created by Rakesh on 19/12/2020.
//

#include "handlers.hpp"
#include "log/NanoLog.hpp"
#include "model/config.hpp"
#include "queue/queuemanager.hpp"
#include "server/s3util.hpp"
#include "util/compress.hpp"

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

spt::server::Response spt::server::get( const http2::framework::RoutingRequest& rr, const boost::container::flat_map<std::string_view, std::string_view>& )
{
  auto resp = Response( rr.req.header );

  const auto& conf = model::Configuration::instance();

  const auto st = std::chrono::steady_clock::now();
  auto metric = model::Metric{};
  try
  {
    metric.method = rr.req.method;
    metric.resource = rr.req.path;
    if ( metric.resource.empty() ) metric.resource = "/";

    if ( const auto pos = metric.resource.find_first_of( '?' ); pos != std::string::npos )
    {
      metric.resource = metric.resource.substr( 0, pos );
    }

    metric.ipaddress = ipaddress( rr.req );
    LOG_DEBUG << "Request from " << metric.ipaddress << " to " << metric.resource;

    // Request path must be absolute and not contain "..".
    if ( metric.resource.empty() || metric.resource[0] != '/' || metric.resource.contains( ".." ) )
    {
      metric.status = 400;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );

      resp.status = 400;
      resp.body = "Illegal request-target";
      return resp;
    }

    // Query strings make no sense in the context of trying to
    // serve files stored in S3 buckets.  If configured as error reject.
    if ( conf.rejectQueryStrings && metric.resource.contains( '?' ) )
    {
      LOG_DEBUG << "Request " << metric.resource << " rejected due to query string policy";
      metric.status = 400;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );

      resp.status = 400;
      resp.body = "Illegal request-target";
      return resp;
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

      resp.status = 404;
      return resp;
    }

    LOG_INFO << "Downloaded resource " << metric.resource << " from S3\n" << downloaded->str();

    // Build the path to the requested file
    std::string path = impl::path_cat( conf.cacheDir, metric.resource );
    if ( metric.resource.back() == '/' ) path.append( "index.html" );

    if ( downloaded->contentType.empty() ) metric.mimeType = std::string{ impl::mime_type( path ) };
    else metric.mimeType = downloaded->contentType;

    const auto compressed = shouldCompress( rr.req ) && impl::should_compress( path );
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

      resp.status = 500;
      resp.body = "I/O error";
      return resp;
    }

    // Handle the case where the file doesn't exist
    if ( !std::filesystem::exists( fn, ec ) )
    {
      metric.status = 404;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );

      resp.status = 404;
      return resp;
    }

    metric.size = size;
    metric.compressed = compressed;

    resp.headers = nghttp2::asio_http2::header_map{
        {"Access-Control-Allow-Origin", {"*", false}},
        {"Access-Control-Allow-Methods", {"GET,HEAD,OPTIONS", false}},
        {"Access-Control-Allow-Headers", {"*, authorization", false}},
        {"content-type", { metric.mimeType, false } },
        { "etag", { downloaded->etag, false } },
        { "expires", { downloaded->expirationTime(), false } },
        { "last-modified", { downloaded->lastModifiedTime(), false } }
    };

    if ( !downloaded->cacheControl.empty() )
    {
      resp.headers.emplace( "cache-control", nghttp2::asio_http2::header_value{ downloaded->cacheControl, false } );
    }

    // Respond to HEAD request
    if ( rr.req.method == "HEAD" )
    {
      resp.headers.emplace( "content-length", nghttp2::asio_http2::header_value{ std::to_string( downloaded->contentLength ), false } );
      metric.status = 200;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );

      resp.status = 200;
      return resp;
    }

    auto ifmatch = ifNoneMatch( rr.req );
    if ( downloaded->etag == ifmatch )
    {
      metric.status = 304;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );

      resp.status = 304;
      return resp;
    }

    auto ifmodified = ifModifiedSince( rr.req );
    if ( downloaded->lastModifiedTime() == ifmodified )
    {
      metric.status = 304;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );

      resp.status = 304;
      return resp;
    }

    if ( compressed )
    {
      resp.headers.emplace( "content-encoding", nghttp2::asio_http2::header_value{ "gzip", false } );
    }
    resp.headers.emplace( "content-length",
        nghttp2::asio_http2::header_value{ std::to_string( metric.size ), false } );

    metric.status = 200;
    const auto et = std::chrono::steady_clock::now();
    const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
    metric.time = delta.count();
    queue::QueueManager::instance().publish( std::move( metric ) );

    resp.status = 200;
    resp.filePath = fn;
    return resp;
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error processing request " << ex.what();
    resp.status = 500;
    resp.body = "Unknown error";
  }

  return resp;
}

spt::server::Response spt::server::clear( const http2::framework::RoutingRequest& rr, const boost::container::flat_map<std::string_view, std::string_view>& )
{
  auto resp = Response( rr.req.header );

  const auto st = std::chrono::steady_clock::now();
  auto metric = model::Metric{};

  try
  {
    metric.method = rr.req.method;
    metric.resource = rr.req.path;
    if ( metric.resource.empty() ) metric.resource = "/";

    if ( const auto pos = metric.resource.find_first_of( '?' ); pos != std::string::npos )
    {
      metric.resource = metric.resource.substr( 0, pos );
    }

    metric.ipaddress = ipaddress( rr.req );
    LOG_DEBUG << "Request from " << metric.ipaddress << " to " << metric.resource;

    auto bearer = authorise( rr.req );
    if ( bearer.empty() )
    {
      metric.status = 403;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      resp.status = 403;
      return resp;
    }

    const auto prefix = std::string("Bearer ");
    if ( !boost::algorithm::starts_with( bearer, prefix ) )
    {
      resp.status = 400;
      resp.body = "Invalid Authorization header";
      metric.status = 400;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return resp;
    }

    std::string temp{ bearer };
    temp.erase( 0, prefix.size() );

    if ( S3Util::instance().clear( temp ) )
    {
      resp.status = 304;
      metric.status = 304;
      const auto et = std::chrono::steady_clock::now();
      const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
      metric.time = delta.count();
      queue::QueueManager::instance().publish( std::move( metric ) );
      return resp;
    }

    resp.status = 403;
    metric.status = 403;
    const auto et = std::chrono::steady_clock::now();
    const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - st );
    metric.time = delta.count();
    queue::QueueManager::instance().publish( std::move( metric ) );
    return resp;
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Error processing request " << ex.what();
    resp.status = 500;
    resp.body = "Unknown error";
  }

  return resp;
}
