//
// Created by Rakesh on 17/05/2020.
//

#include "queuemanager.h"

using spt::util::QueueManager;
using spt::util::Metric;

void QueueManager::publish( Metric metric )
{
  if ( util::publishMetrics( *configuration ) )
  {
    queue.put( PolyM::DataMsg<Metric>( metric.time.count(), std::move( metric ) ) );
  }
}

std::unique_ptr<PolyM::Msg> spt::util::QueueManager::get()
{
  return util::publishMetrics( *configuration ) ? queue.get( 100 ) : nullptr;
}

Metric* spt::util::from( PolyM::Msg* msg )
{
  if ( !msg ) return nullptr;
  auto* dm = dynamic_cast<PolyM::DataMsg<Metric>*>( msg );
  return dm ? &(dm->getPayload()) : nullptr;
}
