//
// Created by Rakesh on 19/12/2020.
//

#include "handlers.h"
#include "log/NanoLog.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>

void spt::server::cors( const nghttp2::asio_http2::server::response& res )
{
  auto headers = nghttp2::asio_http2::header_map{
      {"Access-Control-Allow-Origin", {"*", false}},
      {"Access-Control-Allow-Methods", {"GET,OPTIONS", false}}
  };

  res.write_head(204, headers);
}

auto spt::server::compress( std::string_view data ) -> Output
{
  namespace bio = boost::iostreams;

  if ( data.size() < 128 )
  {
    LOG_INFO << "Uncompressed size: " << int( data.size() ) << " less than 128, not compressing";
    return { {}, false };
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
    // Trim any port part from address
    const auto pos = iter->second.value.find( ':' );
    return pos != std::string::npos ? iter->second.value.substr( 0, pos ) :
        iter->second.value;
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
