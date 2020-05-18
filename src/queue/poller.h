//
// Created by Rakesh on 17/05/2020.
//

#pragma once

#include "model/config.h"

#include <atomic>
#include <memory>

#include <boost/asio/io_context.hpp>

namespace spt::queue
{
  namespace tsdb
  {
    struct TSDBClient;
  }

  class Poller
  {
  public:
    explicit Poller( model::Configuration::Ptr configuration );
    ~Poller();

    Poller( const Poller& ) = delete;
    Poller& operator=( const Poller& ) = delete;

    void run();
    void stop() { running.store( false ); }

  private:
    void loop();

    boost::asio::io_context ioc;
    model::Configuration::Ptr configuration;
    std::unique_ptr<tsdb::TSDBClient> client;
    int64_t count = 0;
    std::atomic_bool running;
  };
}
