//
// Created by Rakesh on 19/12/2020.
//

#include "handlers.hpp"
#include "model/config.hpp"
#include "log/NanoLog.hpp"

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

std::string spt::server::authorise( const http2::framework::Request& req )
{
  auto iter = req.header.find( "authorization" );
  if ( iter == std::cend( req.header ) ) iter = req.header.find( "Authorization" );
  return iter == std::cend( req.header ) ? std::string{} : iter->second.value;
}

bool spt::http2::framework::shouldCompress( const Request& req )
{
  auto iter = req.header.find( "accept-encoding" );
  if ( iter == std::cend( req.header ) ) iter = req.header.find( "Accept-Encoding" );
  return !( iter == std::cend( req.header ) ) && iter->second.value.find( "gzip" ) != std::string::npos;
}

std::string spt::http2::framework::ipaddress( const Request& req )
{
  auto& header = req.header;
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

  return req.remoteEndpoint;
}

std::string spt::server::ifNoneMatch( const http2::framework::Request& req )
{
  auto iter = req.header.find( "if-none-match" );
  if ( iter == std::cend( req.header ) ) iter = req.header.find( "If-None-Match" );
  return iter == std::cend( req.header ) ? std::string{} : iter->second.value;
}

std::string spt::server::ifModifiedSince( const http2::framework::Request& req )
{
  auto iter = req.header.find( "if-modified-since" );
  if ( iter == std::cend( req.header ) ) iter = req.header.find( "If-Modified-Since" );
  return iter == std::cend( req.header ) ? std::string{} : iter->second.value;
}
