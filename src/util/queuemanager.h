//
// Created by Rakesh on 17/05/2020.
//

#pragma once

#include "config.h"
#include "metric.h"
#include "polym/Queue.hpp"

namespace spt::util
{
  class QueueManager
  {
  public:
    static QueueManager& instance( Configuration::Ptr configuration = nullptr )
    {
      static QueueManager mgr( std::move( configuration ) );
      return mgr;
    }

    void publish( Metric metric );
    std::unique_ptr<PolyM::Msg> get();

    ~QueueManager() = default;
    QueueManager( const QueueManager& ) = delete;
    QueueManager& operator=( const QueueManager& ) = delete;

  private:
    explicit QueueManager( Configuration::Ptr configuration ) :
      configuration{ std::move( configuration ) } {}

    PolyM::Queue queue;
    Configuration::Ptr configuration;
  };

  Metric* from( PolyM::Msg* msg );
}
