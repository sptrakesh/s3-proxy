//
// Created by Rakesh on 14/05/2020.
//

#include "mmdb.h"
#include "log/NanoLog.h"

#include <vector>

#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace spt::client::mmdb
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

  struct Worker
  {
    Worker( util::Configuration::Ptr config ) : resolver{ ioc }, ws{ ioc },
      config{ std::move( config ) }
    {
    }

    ~Worker()
    {
      try
      {
        if ( ws.is_open() ) ws.close( websocket::close_code::normal );
      }
      catch ( const std::exception& ex )
      {
        LOG_WARN << "Error closing websocket connection " << ex.what();
      }
    }

    MMDBClient::Properties fields( const std::string& ip )
    {
      MMDBClient::Properties  props;

      try
      {
        connect();
        const std::string req = "f:" + ip;
        ws.write( net::buffer( req ) );
        beast::flat_buffer buffer;
        ws.read( buffer );

        std::ostringstream ss;
        ss << beast::make_printable( buffer.data() );
        const auto text = ss.str();
        if ( text == ip ) return props;

        for ( auto line : split( text, 10, "\n" ) )
        {
          const auto parts = split( line, 2, ":" );
          auto key = std::string{ parts[0].data(), parts[0].size() };
          boost::algorithm::trim( key );
          auto value = std::string{ parts[1].data(), parts[1].size() };
          boost::algorithm::trim( value );
          props.emplace( std::move( key ), std::move( value ) );
        }
      }
      catch ( const std::exception& ex )
      {
        LOG_WARN << "Error querying ip " << ip << ' ' << ex.what();
      }

      return props;
    }

  private:
    void connect()
    {
      try
      {
        if ( ws.is_open() ) return;

        auto const results = resolver.resolve( config->mmdbHost, std::to_string( config->mmdbPort ) );
        net::connect( ws.next_layer(), results.begin(), results.end() );

        ws.set_option(websocket::stream_base::decorator(
            []( websocket::request_type& req )
            {
              req.set( http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket" );
            }));

        ws.handshake( config->mmdbHost, "/" );
        LOG_INFO << "Started websocket connection to " << config->mmdbHost;
      }
      catch ( const std::exception& ex )
      {
        LOG_CRIT << "Error initiating MMDB websocket connection " << ex.what();
      }
    }

    net::io_context ioc;
    tcp::resolver resolver;
    websocket::stream<tcp::socket> ws;
    util::Configuration::Ptr config;
  };
}

using spt::client::MMDBClient;

MMDBClient::~MMDBClient()
{
  delete worker;
}

MMDBClient& MMDBClient::instance( spt::util::Configuration::Ptr config )
{
  static MMDBClient client( std::move( config ) );
  return client;
}

MMDBClient::MMDBClient( spt::util::Configuration::Ptr config ) :
  worker{ new mmdb::Worker( std::move( config ) ) } {}

MMDBClient::Properties MMDBClient::query( const std::string& ip )
{
  return worker->fields( ip );
}
