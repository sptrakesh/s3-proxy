//
// Created by Rakesh on 26/01/2025.
//

#include "response.hpp"
#include "log/NanoLog.hpp"

#include <fmt/ranges.h>

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
          { "content-type", { "application/json; charset=utf-8", false } },
          { "content-length", { std::to_string( body.size() ), false } }
  };
  if ( compressed )
  {
    headers.emplace( "content-encoding", nghttp2::asio_http2::header_value{ "gzip", false } );
  }

  if ( origin.empty() ) return;
  const auto iter = std::ranges::find( origins, origin );
  if ( iter != std::ranges::end( origins ) )
  {
    headers.emplace( "Access-Control-Allow-Origin", nghttp2::asio_http2::header_value{ origin, false } );
    headers.emplace( "Vary", nghttp2::asio_http2::header_value{ "Origin", false } );
  }
  else LOG_WARN << "Origin " << origin << " not allowed";
}
