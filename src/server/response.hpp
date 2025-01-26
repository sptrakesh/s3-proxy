//
// Created by Rakesh on 26/01/2025.
//

#pragma once

#include <span>
#include <nghttp2/asio_http2_server.h>

namespace spt::server
{
  struct Response
  {
    Response( const nghttp2::asio_http2::header_map& headers );
    ~Response() = default;
    Response(Response&&) = default;
    Response& operator=(Response&&) = default;

    Response(const Response&) = delete;
    Response& operator=(const Response&) = delete;

    void set( std::span<const std::string> methods, std::span<const std::string> origins );

    nghttp2::asio_http2::header_map headers;
    std::string body;
    std::string filePath;
    std::string origin;
    uint16_t status{ 200 };
    bool compressed{ false };
  };
}