//
// Created by Rakesh on 17/05/2020.
//

#pragma once

#include "concurrentqueue.h"
#include "model/config.h"
#include "model/metric.h"

namespace spt::queue
{
  class QueueManager
  {
  public:
    static QueueManager& instance();

    void publish( model::Metric metric );
    bool consume( model::Metric& metric );

    ~QueueManager() = default;
    QueueManager( const QueueManager& ) = delete;
    QueueManager& operator=( const QueueManager& ) = delete;

  private:
    explicit QueueManager( const model::Configuration& configuration );

    // Akumuli does not support out of sequence writes, so use a simple queue
    moodycamel::ConcurrentQueue<model::Metric> queue;
    const bool enabled;
  };
}
