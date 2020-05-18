//
// Created by Rakesh on 17/05/2020.
//

#pragma once

#include "polym/Queue.hpp"
#include "model/config.h"
#include "model/metric.h"

namespace spt::queue
{
  class QueueManager
  {
  public:
    static QueueManager& instance( model::Configuration* configuration = nullptr )
    {
      static QueueManager mgr( configuration );
      return mgr;
    }

    void publish( model::Metric metric );
    std::unique_ptr<PolyM::Msg> get();

    ~QueueManager() = default;
    QueueManager( const QueueManager& ) = delete;
    QueueManager& operator=( const QueueManager& ) = delete;

  private:
    explicit QueueManager( model::Configuration* configuration ) :
      enabled{ model::publishMetrics( *configuration ) } {}

    PolyM::Queue queue;
    bool enabled;
  };

  model::Metric* from( PolyM::Msg* msg );
}
