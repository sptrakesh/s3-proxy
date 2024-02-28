//
// Created by Rakesh on 19/12/2020.
//

#include "handlers.h"
#include "model/config.h"
#include "log/NanoLog.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/json/parse.hpp>

#include <cstdlib>
#include <vector>
#include <fmt/ranges.h>
#include <range/v3/algorithm/find.hpp>

using std::operator""sv;

namespace spt::server::phandlers
{
  struct Holder
  {
    static Holder& instance()
    {
      static Holder h;
      return h;
    }

    std::vector<std::string> origins;

  private:
    Holder()
    {
      if ( const char* env_p = std::getenv( "ALLOWED_ORIGINS" ) )
      {
        auto ec = boost::system::error_code{};
        auto p = boost::json::parse( env_p, ec );
        if ( ec )
        {
          LOG_WARN << "Error parsing allowed origins from environment variable " << env_p;
          return;
        }

        if ( !p.is_array() )
        {
          LOG_CRIT << "Invalid value for ALLOWED_ORIGINS.  Expected array.";
          return;
        }

        auto& arr = p.as_array();
        origins.reserve( arr.size() );
        for ( const auto& origin : arr ) origins.emplace_back( origin.as_string() );
        LOG_INFO << fmt::format( "Parsed allowed origins {}", origins );
      }
    }
  };
}

void spt::server::cors( const nghttp2::asio_http2::header_map& headers, const nghttp2::asio_http2::server::response& res )
{
  auto hdrs = nghttp2::asio_http2::header_map{
      { "Access-Control-Allow-Methods", { "GET,OPTIONS", false } },
      { "Access-Control-Allow-Headers", { "*, authorization", false } },
      { "content-length", { "0", false } }
  };

  if ( const auto iter = headers.find( "origin" ); iter != std::cend( headers ) )
  {
    const auto& origins = phandlers::Holder::instance().origins;
    if ( origins.empty() )
    {
      hdrs.emplace( "Access-Control-Allow-Origin", nghttp2::asio_http2::header_value{ iter->second.value, false } );
      hdrs.emplace( "Vary", nghttp2::asio_http2::header_value{ "Origin", false } );
    }
    else
    {
      const auto oiter = ranges::find( origins, iter->second.value );
      if ( oiter != ranges::end( origins ) )
      {
        hdrs.emplace( "Access-Control-Allow-Origin", nghttp2::asio_http2::header_value{ iter->second.value, false } );
        hdrs.emplace( "Vary", nghttp2::asio_http2::header_value{ "Origin", false } );
      }
      else
      {
        LOG_WARN << "Origin " << iter->second.value << " not allowed.  Configured origins: " << fmt::format( "{}", phandlers::Holder::instance().origins );
      }
    }
  }

  res.write_head( 204, std::move( hdrs ) );
}

auto spt::server::compress( std::string_view data ) -> Output
{
  namespace bio = boost::iostreams;

  if ( data.size() < 128 )
  {
    LOG_INFO << "Uncompressed size: " << int( data.size() ) << " less than 128, not compressing";
    return { std::string{}, false };
  }

  bio::stream<bio::array_source> source( data.data(), data.size() );
  std::ostringstream compressed;
  bio::filtering_streambuf<bio::input> in;
  in.push( bio::gzip_compressor( bio::gzip_params( bio::gzip::best_compression ) ) );
  in.push( source );
  bio::copy( in, compressed );

  auto str = compressed.str();
  LOG_INFO << "Uncompressed size: " << int( data.size() ) << " compressed size: " << int( str.size() );
  return { std::move( str ), true };
}

std::string spt::server::authorise( const nghttp2::asio_http2::server::request& req )
{
  auto iter = req.header().find( "authorization" );
  if ( iter == std::cend( req.header() ) ) iter = req.header().find( "Authorization" );
  return iter == std::cend( req.header() ) ? std::string{} : iter->second.value;
}

bool spt::server::shouldCompress( const nghttp2::asio_http2::server::request& req )
{
  auto iter = req.header().find( "accept-encoding" );
  if ( iter == std::cend( req.header() ) ) iter = req.header().find( "Accept-Encoding" );
  return !( iter == std::cend( req.header() ) ) &&
      iter->second.value.find( "gzip" ) != std::string::npos;
}

std::string spt::server::ipaddress( const nghttp2::asio_http2::server::request& req )
{
  auto& header = req.header();
  auto iter = header.find( "x-real-ip" );
  if ( iter == std::cend( header ) ) iter = header.find( "x-forwarded-for" );
  if ( iter != std::cend( header ) )
  {
    const auto trimmed = boost::algorithm::trim_copy( iter->second.value );
    std::vector<std::string> parts;
    parts.reserve( 4 );
    boost::split( parts, trimmed, [](char c){return c == ',' || c == ' ';});

    boost::system::error_code ec;
    boost::asio::ip::address::from_string( parts[0], ec );
    if ( !ec ) return parts[0];

    // Trim any port part from address
    const auto pos = parts[0].find( ':' );
    if ( pos != std::string::npos )
    {
      auto val = parts[0].substr( 0, pos );
      boost::asio::ip::address::from_string( val, ec );
      if ( !ec ) return val;
    }

    LOG_WARN << "Invalid IP address " << parts[0] << ". " << ec.message();
  }

  return req.remote_endpoint().address().to_string();
}

std::string spt::server::ifNoneMatch( const nghttp2::asio_http2::server::request& req )
{
  auto iter = req.header().find( "if-none-match" );
  if ( iter == std::cend( req.header() ) ) iter = req.header().find( "If-None-Match" );
  return iter == std::cend( req.header() ) ? std::string{} : iter->second.value;
}

std::string spt::server::ifModifiedSince( const nghttp2::asio_http2::server::request& req )
{
  auto iter = req.header().find( "if-modified-since" );
  if ( iter == std::cend( req.header() ) ) iter = req.header().find( "If-Modified-Since" );
  return iter == std::cend( req.header() ) ? std::string{} : iter->second.value;
}

void spt::server::write( int code, std::string body,
    const nghttp2::asio_http2::server::response& res )
{
  auto headers = nghttp2::asio_http2::header_map{
      {"content-type", { "text/plain", false } },
      { "content-length", { std::to_string( body.size() ), false } }
  };
  res.write_head( code, std::move( headers ) );
  res.end( std::move( body ) );
}
