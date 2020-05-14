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
    std::string authKey;
    std::string cacheDir{ "/tmp" };
    std::string mmdbHost;
    int port = 8000;
    int mmdbPort = 8010;
    int ttl = 300;
    int threads = std::thread::hardware_concurrency();

    std::string str() const;
  };
}
