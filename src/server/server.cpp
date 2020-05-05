//
// Created by Rakesh on 05/05/2020.
//

#include "server.h"
#include "worker.h"
#include "log/NanoLog.h"

int spt::server::run( const util::Configuration& configuration )
{
  try
  {
    auto const address = net::ip::make_address( "0.0.0.0" );

    net::io_context ioc{ 1 };
    tcp::acceptor acceptor{ ioc, { address, static_cast<unsigned short>( configuration.port ) } };

    std::list<Worker> workers;
    for (int i = 0; i < configuration.threads; ++i)
    {
      workers.emplace_back( acceptor, configuration.cacheDir );
      workers.back().start();
    }

    LOG_INFO << "HTTP service started";
    ioc.run();
    LOG_INFO << "HTTP service stopped";
  }
  catch ( const std::exception& e )
  {
    LOG_CRIT << "Error: " << e.what();
    return EXIT_FAILURE;
  }

  return 0;
}
