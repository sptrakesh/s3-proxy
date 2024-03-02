//
// Created by Rakesh on 23/10/2019.
//

#include "ilp.h"

#include <thread>
#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>

using spt::client::ILPClient;

ILPClient::ILPClient( boost::asio::io_context& ioc, std::string_view h, std::string_view p ) :
    s{ ioc }, resolver{ ioc },
    host{ h.data(), h.size() }, port{ p.data(), p.size() }
{
  boost::system::error_code ec;
  endpoints = resolver.resolve( host, port, ec );
  if ( ec )
  {
    LOG_CRIT << "Error resolving service " << host << ':' << port << ". " << ec.message();
    throw std::runtime_error{ "Cannot resolve service host:port" };
  }

  boost::asio::connect( s, endpoints, ec );
  if ( ec )
  {
    LOG_CRIT << "Error connecting to service " << host << ':' << port << ". " << ec.message();
    throw std::runtime_error{ "Cannot connect to service host:port" };
  }
  boost::asio::socket_base::keep_alive option( true );
  s.set_option( option );
}

void ILPClient::write( std::string&& data )
{
  std::ostream os{ &buffer };
  os.write( data.data(), data.size() );

  boost::system::error_code ec;
  auto isize = socket().send( buffer.data(), 0, ec );
  if ( ec )
  {
    LOG_DEBUG << "Error sending data to socket " << ec.message();
    s.close( ec );
    isize = socket().send( buffer.data(), 0, ec );
  }

  buffer.consume( isize );
  last = std::chrono::system_clock::now();
}

boost::asio::ip::tcp::socket& ILPClient::socket()
{
  boost::system::error_code ec;

  using namespace std::literals::chrono_literals;

  if ( std::chrono::system_clock::now() - last > 4min )
  {
    s.close( ec );
    if ( ec ) LOG_DEBUG << "Error closing connection. " << ec.message();
  }

  if ( !s.is_open() )
  {
    LOG_DEBUG << "Re-opening closed connection.";
    boost::asio::connect( s, endpoints, ec );
    if ( ec )
    {
      LOG_CRIT << "Error connecting to service " << host << ':' << port << ". " << ec.message();
      throw std::runtime_error{ "Cannot connect to service host:port" };
    }
    boost::asio::socket_base::keep_alive option( true );
    s.set_option( option );
  }

  return s;
}
