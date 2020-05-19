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
    queue.enqueue( std::move( metric ) );
  }
}

bool QueueManager::consume( Metric& metric )
{
  return queue.try_dequeue( metric );
}

