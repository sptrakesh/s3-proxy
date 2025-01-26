//
// Created by Rakesh on 17/05/2020.
//

#include "queuemanager.hpp"
#include "log/NanoLog.hpp"

using spt::queue::QueueManager;
using spt::model::Metric;

QueueManager& QueueManager::instance()
{
  static QueueManager mgr{ model::Configuration::instance() };
  return mgr;
}

QueueManager::QueueManager( const model::Configuration& config ) :
    enabled{ model::publishMetrics( config ) }
{
  LOG_INFO << "Queue manager initialised.  Enabled: " << enabled;
}

void QueueManager::publish( Metric metric )
{
  if ( enabled )
  {
    queue.enqueue( std::move( metric ) );
  }
}

bool QueueManager::consume( Metric& metric )
{
  return queue.try_dequeue( metric );
}

