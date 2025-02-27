//
// Created by Rakesh on 26/01/2025.
//

#pragma once

#include <span>
#include <boost/asio/thread_pool.hpp>
#include <http2/framework/request.hpp>
#include <http2/framework/stream.hpp>

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
    std::string path;
    std::string body;
    std::string filePath;
    std::string origin;
    std::string mimeType;
    std::size_t contentLength{ 0 };
    uint16_t status{ 200 };
    bool compressed{ false };

    static const std::vector<std::string>& origins();
  };
}

namespace spt::http2::framework
{
  template <>
  void extraProcess( const Request& req, spt::server::Response& resp, boost::asio::thread_pool& pool );
}
