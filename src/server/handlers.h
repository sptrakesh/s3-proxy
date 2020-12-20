//
// Created by Rakesh on 19/12/2020.
//

#pragma once

#include <nghttp2/asio_http2_server.h>

namespace spt::server
{
  void handleRoot( const nghttp2::asio_http2::server::request& req,
      const nghttp2::asio_http2::server::response& res );
  void cors( const nghttp2::asio_http2::server::response& res );

  std::string authorise( const nghttp2::asio_http2::server::request& req );
  bool shouldCompress( const nghttp2::asio_http2::server::request& req );
  std::string ipaddress( const nghttp2::asio_http2::server::request& req );
  std::string ifNoneMatch( const nghttp2::asio_http2::server::request& req );
  std::string ifModifiedSince( const nghttp2::asio_http2::server::request& req );

  using Output = std::tuple<std::string,bool>;
  Output compress( std::string_view data );

  void write( int code, std::string data,
      const nghttp2::asio_http2::server::response& res );
}
