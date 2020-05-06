//
// Created by Rakesh on 2019-05-20.
//

#pragma once

#include "ExpirationCache.h"
#include "s3object.h"

namespace spt::util
{
  using MetadataCache = ExpirationCache<std::string, S3Object, CACHE_TTL>;

  inline MetadataCache& getMetadataCache()
  {
    static MetadataCache cache;
    return cache;
  }
}
