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

#include <boost/asio/signal_set.hpp>

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
    cfg.numberOfServerThreads = configuration.threads;

    if ( const char* st = std::getenv( "SINGLE_THREADED" ) )
    {
      if ( std::string_view{ st } == "true" ) cfg.numberOfServerThreads = 1;
    }

    cfg.numberOfWorkerThreads = 2 * configuration.threads;
    cfg.origins = Response::origins();
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
    signals.async_wait( [&ch](auto const&, int ) { ch.ioc.stop(); } );

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
    server.stop();
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
