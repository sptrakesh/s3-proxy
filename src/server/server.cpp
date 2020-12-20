//
// Created by Rakesh on 05/05/2020.
//

#include "contextholder.h"
#include "log/NanoLog.h"
#include "queue/poller.h"
#include "queue/queuemanager.h"
#include "server/handlers.h"
#include "server/server.h"
#include "server/s3util.h"

#include <cstdlib>
#include <iostream>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>

#include <nghttp2/asio_http2_server.h>

int spt::server::run()
{
  try
  {
    const auto& configuration = model::Configuration::instance();
    auto& ch = ContextHolder::instance();
    S3Util::instance();
    LOG_INFO << "Initialised AWS S3 Util";

    nghttp2::asio_http2::server::http2 server;
    server.num_threads( configuration.threads );

    server.handle( "/", &spt::server::handleRoot );

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
    v.reserve( configuration.threads );
    for ( auto i = configuration.threads - 1; i > 0; --i )
    {
      v.emplace_back( [&ch] { ch.ioc.run(); } );
    }

    auto poller = queue::Poller{};

    if ( model::publishMetrics( configuration ) )
    {
      v.emplace_back( std::thread{ &spt::queue::Poller::run, &poller } );
    }

    boost::system::error_code ec;
    if ( server.listen_and_serve( ec, "0.0.0.0", std::to_string( configuration.port ), true ) ) {
      LOG_CRIT << "error: " << ec.message();
      return 1;
    }

    LOG_INFO << "HTTP/2 service started";
    ch.ioc.run();

    LOG_INFO << "HTTP/2 service stopped";
    poller.stop();
    for ( auto& t : v ) t.join();
    server.stop();
    server.join();

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
