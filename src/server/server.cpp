//
// Created by Rakesh on 05/05/2020.
//

#include "contextholder.hpp"
#include "log/NanoLog.hpp"
#include "queue/poller.hpp"
#include "queue/queuemanager.hpp"
#include "server/handlers.hpp"
#include "server/response.hpp"
#include "server/server.hpp"
#include "server/s3util.hpp"

#include <cstdlib>
#include <iostream>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json/parse.hpp>

namespace
{
  namespace pserver
  {
    std::vector<std::string> origins()
    {
      auto vec = std::vector<std::string>{};

      if ( const char* env_p = std::getenv( "ALLOWED_ORIGINS" ) )
      {
        auto ec = boost::system::error_code{};
        auto p = boost::json::parse( env_p, ec );
        if ( ec )
        {
          LOG_WARN << "Error parsing allowed origins from environment variable " << env_p << ". " << ec.message();
          return vec;
        }

        if ( !p.is_array() )
        {
          LOG_CRIT << "Invalid value for ALLOWED_ORIGINS.  Expected array.";
          return vec;
        }

        const auto& arr = p.as_array();
        vec.reserve( arr.size() );
        for ( const auto& origin : arr ) vec.emplace_back( origin.as_string() );
        LOG_INFO << fmt::format( "Parsed allowed origins {}", fmt::join( vec, ", " ) );
      }
      else LOG_INFO << "Allowed origins not configured";

      return vec;
    }
  }
}

int spt::server::run()
{
  try
  {
    const auto& configuration = model::Configuration::instance();
    auto& ch = ContextHolder::instance();
    S3Util::instance();
    LOG_INFO << "Initialised AWS S3 Util";

    auto cfg = spt::http2::framework::Configuration{};
    cfg.port = static_cast<uint16_t>( configuration.port );
    cfg.numberOfWorkerThreads = configuration.threads;
    cfg.origins = pserver::origins();
    cfg.corsMethods.clear();
    cfg.corsMethods.emplace_back( "GET" );
    cfg.corsMethods.emplace_back( "HEAD" );
    cfg.corsMethods.emplace_back( "OPTIONS" );

    auto server = spt::http2::framework::Server<Response>{ cfg };

    server.addHandler( "GET", "/", &get );
    server.addHandler( "HEAD", "/", &get );
    server.addHandler( "GET", "/*", &get );
    server.addHandler( "HEAD", "/*", &get );
    server.addHandler( "DELETE", "/_proxy/_private/_cache/clear/", &clear );

    LOG_INFO << "Starting HTTP/2 server on localhost:" << configuration.port;
    server.start();

    boost::asio::signal_set signals( ch.ioc, SIGINT, SIGTERM );
    signals.async_wait( [&ch, &server](auto const&, int ) { server.stop(); ch.ioc.stop(); } );

    boost::asio::signal_set sigpipe( ch.ioc, SIGPIPE );
    sigpipe.async_wait( [](auto const& ec, int code )
        {
          LOG_CRIT << "Sigpipe received with code: " << code << ". " << ec.message();
        }
    );

    queue::QueueManager::instance();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.resize( 2 );
    v.emplace_back( [&ch] { ch.ioc.run(); } );

    auto poller = queue::Poller{};
    if ( model::publishMetrics( configuration ) ) v.emplace_back( &queue::Poller::run, &poller );
    ch.ioc.run();

    LOG_INFO << "HTTP/2 service stopped";
    poller.stop();
    for ( auto& t : v ) if ( t.joinable() ) t.join();

    LOG_INFO << "All I/O threads stopped";
    return EXIT_SUCCESS;
  }
  catch ( const std::exception& e )
  {
    std::cerr << e.what() << '\n';
    LOG_CRIT << "Error: " << e.what();
    return EXIT_FAILURE;
  }
}
