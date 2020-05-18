//
// Created by Rakesh on 05/05/2020.
//

#include "server.h"
#include "stackless.h"
#include "client/mmdb.h"
#include "log/NanoLog.h"
#include "queue/poller.h"
#include "queue/queuemanager.h"

#include <iostream>
#include <boost/asio/signal_set.hpp>

int spt::server::run( model::Configuration::Ptr configuration )
{
  try
  {
    S3Util::instance( configuration );
    LOG_INFO << "Initialised AWS S3 Util";

    auto const address = net::ip::make_address( "0.0.0.0" );
    net::io_context ioc{ configuration->threads };

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&](beast::error_code const&, int)
        {
          ioc.stop();
        });

    queue::QueueManager::instance( configuration.get() );

    // Create and launch a listening port
    std::make_shared<listener>( ioc,
        tcp::endpoint{ address, static_cast<unsigned short>( configuration->port ) },
        configuration.get() )->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve( configuration->threads );
    for ( auto i = configuration->threads - 1; i > 0; --i )
    {
      v.emplace_back( [&ioc] { ioc.run(); } );
    }

    auto poller = queue::Poller{ configuration };

    if ( model::publishMetrics( *configuration ) )
    {
      client::MMDBClient::instance( configuration );
      v.emplace_back( std::thread{ &spt::queue::Poller::run, &poller } );
    }

    LOG_INFO << "HTTP service started";

    ioc.run();
    LOG_INFO << "HTTP service stopped";

    poller.stop();
    for ( auto& t : v ) t.join();

    LOG_INFO << "All I/O threads stopped";
    return EXIT_SUCCESS;
  }
  catch ( const std::exception& e )
  {
    LOG_CRIT << "Error: " << e.what();
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
