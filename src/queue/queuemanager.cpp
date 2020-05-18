//
// Created by Rakesh on 17/05/2020.
//

#include "queuemanager.h"

using spt::queue::QueueManager;
using spt::model::Metric;

void QueueManager::publish( Metric metric )
{
  if ( enabled )
  {
    queue.put( PolyM::DataMsg<Metric>( metric.time.count(), std::move( metric ) ) );
  }
}

std::unique_ptr<PolyM::Msg> QueueManager::get()
{
  return enabled ? queue.get( 1000 ) : nullptr;
}

Metric* spt::queue::from( PolyM::Msg* msg )
{
  if ( !msg ) return nullptr;
  auto* dm = dynamic_cast<PolyM::DataMsg<Metric>*>( msg );
  return dm ? &( dm->getPayload() ) : nullptr;
}
