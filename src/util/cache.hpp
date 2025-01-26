//
// Created by Rakesh on 2019-05-20.
//

#pragma once

#include "ExpirationCache.hpp"
#include "model/s3object.hpp"

namespace spt::util
{
  using MetadataCache = ExpirationCache<std::string, model::S3Object::Ptr, CACHE_TTL>;
  using LocationCache = ExpirationCache<std::string, std::string, CACHE_TTL>;

  inline MetadataCache& getMetadataCache()
  {
    static MetadataCache cache;
    return cache;
  }

  inline LocationCache& getLocationCache()
  {
    static LocationCache cache;
    return cache;
  }
}
