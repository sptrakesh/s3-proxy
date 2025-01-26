//
// Created by Rakesh on 2019-05-24.
//
// https://github.com/bjlaub/simpletsdb-client

#pragma once

#include <chrono>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

#if defined __has_include
#if __has_include("../log/NanoLog.hpp")
#include "../log/NanoLog.hpp"
#else
#include <log/NanoLog.h>
#endif
#endif

namespace spt::client
{
  struct ILPClient
  {
    ILPClient( boost::asio::io_context& context, std::string_view host, std::string_view port );

    void write( std::string&& ilp );

    ILPClient( const ILPClient& ) = delete;
    ILPClient& operator=( const ILPClient& ) = delete;

    ~ILPClient()
    {
      boost::system::error_code ec;
      s.close( ec );
      if ( ec ) LOG_CRIT << "Error closing socket. " << ec.message();
    }

  private:
    boost::asio::ip::tcp::socket& socket();

    boost::asio::ip::tcp::socket s;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::tcp::resolver::results_type endpoints;
    boost::asio::streambuf buffer;
    std::string host;
    std::string port;
    std::chrono::time_point<std::chrono::system_clock> last{ std::chrono::system_clock::now() };
  };
}
