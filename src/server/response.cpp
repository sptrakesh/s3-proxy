//
// Created by Rakesh on 26/01/2025.
//

#include "response.hpp"
#include "log/NanoLog.hpp"
#include "model/metric.hpp"
#include "queue/queuemanager.hpp"

#include <boost/json/parse.hpp>
#include <fmt/ranges.h>

namespace
{
  namespace presponse
  {
    std::vector<std::string> origins()
    {
      auto vec = std::vector<std::string>{};

      if ( const char* env_p = std::getenv( "ALLOWED_ORIGINS" ) )
      {
        auto ec = boost::system::error_code{};
        auto p = boost::json::parse( env_p, ec );
        if ( ec )
        {
          LOG_WARN << "Error parsing allowed origins from environment variable " << env_p << ". " << ec.message();
          return vec;
        }

        if ( !p.is_array() )
        {
          LOG_CRIT << "Invalid value for ALLOWED_ORIGINS.  Expected array.";
          return vec;
        }

        const auto& arr = p.as_array();
        vec.reserve( arr.size() );
        for ( const auto& origin : arr ) vec.emplace_back( origin.as_string() );
        LOG_INFO << fmt::format( "Parsed allowed origins {}", fmt::join( vec, ", " ) );
      }
      else LOG_INFO << "Allowed origins not configured";

      return vec;
    }
  }
}

using spt::server::Response;

Response::Response( const nghttp2::asio_http2::header_map& headers )
{
  auto iter = headers.find( "origin" );
  if ( iter == std::cend( headers ) ) iter = headers.find( "Origin" );
  if ( iter == std::cend( headers ) ) return;
  origin = iter->second.value;
}

void Response::set( std::span<const std::string> methods, std::span<const std::string> origins )
{
  headers = nghttp2::asio_http2::header_map{
          { "Access-Control-Allow-Methods", { fmt::format( "{}", fmt::join( methods, ", " ) ), false } },
          { "Access-Control-Allow-Headers", { "*, authorization", false } },
          { "content-length", { std::to_string( body.size() ), false } }
  };
  if ( compressed )
  {
    headers.emplace( "content-encoding", nghttp2::asio_http2::header_value{ "gzip", false } );
  }

  if ( origin.empty() ) return;
  if ( origins.empty() )
  {
    headers.emplace( "Access-Control-Allow-Origin", nghttp2::asio_http2::header_value{ origin, false } );
    headers.emplace( "Vary", nghttp2::asio_http2::header_value{ "Origin", false } );
    return;
  }

  const auto iter = std::ranges::find( origins, origin );
  if ( iter != std::ranges::end( origins ) )
  {
    headers.emplace( "Access-Control-Allow-Origin", nghttp2::asio_http2::header_value{ origin, false } );
    headers.emplace( "Vary", nghttp2::asio_http2::header_value{ "Origin", false } );
  }
  else LOG_WARN << "Origin " << origin << " not allowed";
}

const std::vector<std::string>& Response::origins()
{
  static const auto vec = presponse::origins();
  return vec;
}

template <>
void spt::http2::framework::extraProcess( const Request& req, server::Response& response, boost::asio::thread_pool& )
{
  auto metric = model::Metric{};
  metric.method = req.method;
  metric.status = response.status;
  metric.mimeType = response.mimeType;
  metric.resource = response.path;

  metric.ipaddress = ipaddress( req );
  LOG_DEBUG << "Request from " << metric.ipaddress << " to " << response.path;

  const auto et = std::chrono::high_resolution_clock::now();
  const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>( et - req.timestamp );
  metric.time = delta.count();
  queue::QueueManager::instance().publish( std::move( metric ) );
}
