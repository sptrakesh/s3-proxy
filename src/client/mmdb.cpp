//
// Created by Rakesh on 16/12/2020.
//

#include "mmdb.hpp"
#include "contextholder.hpp"
#include "log/NanoLog.hpp"
#include "model/config.hpp"

#include <vector>

#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read_until.hpp>

namespace spt::client::pmmdb
{
  std::vector<std::string_view> split( std::string_view text,
      std::size_t sizehint = 8, std::string_view delims = "\n" )
  {
    std::vector<std::string_view> output;
    output.reserve( sizehint );
    auto first = text.cbegin();

    while ( first != text.cend() )
    {
      const auto second = std::find_first_of( first, std::cend( text ),
          std::cbegin( delims ), std::cend( delims ) );

      if ( first != second )
      {
        output.emplace_back(
            text.substr( std::distance( text.begin(), first ),
                std::distance( first, second ) ) );
      }

      if ( second == text.cend() ) break;
      first = std::next( second );
    }

    return output;
  }
}

using spt::client::MMDBConnection;
using spt::client::MMDBConnectionPool;

MMDBConnection::MMDBConnection( boost::asio::io_context& ioc, std::string_view h, std::string_view p ) :
  s{ ioc }, resolver( ioc ),
  host{ h.data(), h.size() }, port{ p.data(), p.size() }
{
  boost::asio::connect( s, resolver.resolve( host, port ) );
}

MMDBConnection::Properties MMDBConnection::fields( std::string_view ip )
{
  MMDBConnection::Properties  props;
  std::string text;

  try
  {
    text = execute( ip );
    if ( text == ip ) return props;

    for ( auto line : pmmdb::split( text, 10, "\n" ) )
    {
      const auto parts = pmmdb::split( line, 2, ":" );
      auto key = std::string{ parts[0].data(), parts[0].size() };
      boost::algorithm::trim( key );
      auto value = std::string{ parts[1].data(), parts[1].size() };
      boost::algorithm::trim( value );
      props.emplace( std::move( key ), std::move( value ) );
    }
  }
  catch ( const std::exception& ex )
  {
    LOG_WARN << "Error parsing ip response " << text << '\n' << ex.what();
  }

  return props;
}

std::string MMDBConnection::execute( std::string_view query )
{
  std::ostream os{ &buffer };
  os << "f:" << query << '\n';

  const auto isize = socket().send( buffer.data() );
  buffer.consume( isize );

  boost::system::error_code ec;
  auto osize = boost::asio::read_until( socket(), buffer, "\n\n", ec );
  if ( ec )
  {
    LOG_WARN << "Error reading from socket " << ec.message() << '\n';
    return {};
  }

  buffer.commit( osize );

  auto resp = std::string{ reinterpret_cast<const char*>( buffer.data().data() ), osize };
  boost::algorithm::trim( resp );
  resp.erase( std::remove( std::begin( resp ), std::end( resp ), '\0' ), std::end( resp ) );

  buffer.consume( buffer.size() );
  return resp;
}

std::unique_ptr<MMDBConnection> spt::client::createMMDBPool()
{
  const auto& conf = model::Configuration::instance();
  return std::make_unique<MMDBConnection>(
      ContextHolder::instance().ioc, conf.mmdbHost, std::to_string( conf.mmdbPort ) );
}

boost::asio::ip::tcp::socket& MMDBConnection::socket()
{
  if ( ! s.is_open() ) boost::asio::connect( s, resolver.resolve( host, port ) );
  return s;
}

MMDBConnectionPool& MMDBConnectionPool::instance()
{
  static MMDBConnectionPool instance;
  return instance;
}

auto MMDBConnectionPool::acquire() -> std::optional<Pool<MMDBConnection>::Proxy>
{
  return pool->acquire();
}

MMDBConnectionPool::MMDBConnectionPool() :
  pool{ std::make_unique<Pool<MMDBConnection>>( spt::client::createMMDBPool, Configuration{} ) }
{
}
