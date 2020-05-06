//
// Created by Rakesh on 2019-05-14.
//

#pragma once

#include <memory>
#include <string>
#include <thread>

namespace spt::util
{
  struct Configuration
  {
    using Ptr = std::shared_ptr<Configuration>;

    std::string key;
    std::string secret;
    std::string region;
    std::string bucket;
    std::string cacheDir{ "/tmp" };
    bool cacheInMemory = false;
    int port = 8000;
    int ttl = 300;
    int threads = std::thread::hardware_concurrency();

    std::string str() const;
  };

  std::string hostname();
}
