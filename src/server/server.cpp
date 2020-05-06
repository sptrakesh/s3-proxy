//
// Created by Rakesh on 05/05/2020.
//

#include "server.h"
#include "stackless.h"
#include "log/NanoLog.h"

#include <iostream>
#include <boost/asio/signal_set.hpp>

int spt::server::run( util::Configuration::Ptr configuration )
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

    // Create and launch a listening port
    auto const docroot = std::make_shared<std::string>( configuration->cacheDir );
    std::make_shared<listener>(
        ioc,
        tcp::endpoint{ address, static_cast<unsigned short>( configuration->port ) },
        docroot )->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve( configuration->threads - 1 );
    for( auto i = configuration->threads - 1; i > 0; --i )
    {
      v.emplace_back( [&ioc] { ioc.run(); } );
    }
    LOG_INFO << "HTTP service started";

    ioc.run();
    LOG_INFO << "HTTP service stopped";

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
