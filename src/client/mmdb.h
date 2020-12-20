//
// Created by Rakesh on 16/12/2020.
//

#pragma once

#include "pool.h"

#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace spt::client
{
  struct MMDBConnection
  {
    MMDBConnection( boost::asio::io_context& ioc, std::string_view h, std::string_view p );

    MMDBConnection( const MMDBConnection& ) = delete;
    MMDBConnection& operator=( const MMDBConnection& ) = delete;

    ~MMDBConnection()
    {
      s.close();
    }

    using Properties = std::unordered_map<std::string, std::string>;
    Properties fields( std::string_view ip );

    bool valid() const { return v; }
    void setValid( bool valid ) { this->v = valid; }

  private:
    std::string execute( std::string_view query );
    boost::asio::ip::tcp::socket& socket();

    boost::asio::ip::tcp::socket s;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::streambuf buffer;
    std::string host;
    std::string port;
    bool v{ true };
  };

  std::unique_ptr<MMDBConnection> createMMDBPool();

  struct MMDBConnectionPool
  {
    static MMDBConnectionPool& instance();
    std::optional<Pool<MMDBConnection>::Proxy> acquire();

    ~MMDBConnectionPool() = default;

    MMDBConnectionPool( const MMDBConnectionPool& ) = delete;
    MMDBConnectionPool( MMDBConnectionPool&& ) = delete;
    MMDBConnectionPool& operator=( const MMDBConnectionPool& ) = delete;
    MMDBConnectionPool& operator=( MMDBConnectionPool&& ) = delete;

  private:
    MMDBConnectionPool();

    std::unique_ptr<Pool<MMDBConnection>> pool = nullptr;
  };
}
