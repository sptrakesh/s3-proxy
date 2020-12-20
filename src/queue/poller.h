//
// Created by Rakesh on 17/05/2020.
//

#pragma once

#include <atomic>
#include <memory>

namespace spt::queue
{
  namespace tsdb
  {
    struct MongoClient;
    struct TSDBClient;
  }

  class Poller
  {
  public:
    Poller();
    ~Poller();

    Poller( const Poller& ) = delete;
    Poller& operator=( const Poller& ) = delete;

    void run();
    void stop();

  private:
    void loop();

    std::unique_ptr<tsdb::MongoClient> mongo{ nullptr };
    std::unique_ptr<tsdb::TSDBClient> tsdb{ nullptr };
    int64_t count = 0;
    std::atomic_bool running;
  };
}
