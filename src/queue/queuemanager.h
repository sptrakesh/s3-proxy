//
// Created by Rakesh on 17/05/2020.
//

#pragma once

#include "model/config.h"
#include "model/metric.h"

#include <mutex>
#include <queue>

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
    bool empty() const;
    const model::Metric& front() const;
    void pop();

    ~QueueManager() = default;
    QueueManager( const QueueManager& ) = delete;
    QueueManager& operator=( const QueueManager& ) = delete;

  private:
    explicit QueueManager( model::Configuration* configuration );

    // Akumuli does not support out of sequence writes, so use a simple queue
    std::queue<model::Metric> queue;
    mutable std::mutex mutex;
    const bool enabled;
  };
}
