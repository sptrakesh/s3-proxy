//
// Created by Rakesh on 17/05/2020.
//

#include "queuemanager.h"
#include "log/NanoLog.h"

using spt::queue::QueueManager;
using spt::model::Metric;

QueueManager::QueueManager( spt::model::Configuration* configuration ) :
    enabled{ model::publishMetrics( *configuration ) }
{
  LOG_INFO << "Queue manager initialised.  Enabled: " << enabled;
}

void QueueManager::publish( Metric metric )
{
  if ( enabled )
  {
    std::lock_guard<std::mutex> lg{ mutex };
    queue.push( std::move( metric ) );
  }
}

bool QueueManager::empty() const
{
  std::lock_guard<std::mutex> lg{ mutex };
  return queue.empty();
}

const Metric& QueueManager::front() const
{
  std::lock_guard<std::mutex> lg{ mutex };
  return queue.front();
}

void QueueManager::pop()
{
  std::lock_guard<std::mutex> lg{ mutex };
  queue.pop();
}

