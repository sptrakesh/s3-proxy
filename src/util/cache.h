//
// Created by Rakesh on 2019-05-20.
//

#pragma once

#include "ExpirationCache.h"

namespace spt::util
{
  using SensorIdCache = ExpirationCache<std::string, std::string, 24*60*60>;
  using ReelWithCableIdCache = ExpirationCache<std::string, std::string, 24*60*60>;

  inline SensorIdCache& getSensorIdCache()
  {
    static SensorIdCache cache;
    return cache;
  }

  inline ReelWithCableIdCache& getReelWithCableIdCache()
  {
    static ReelWithCableIdCache cache;
    return cache;
  }
}
