//
// Created by Rakesh on 2019-05-14.
//

#pragma once

#include <memory>
#include <string>
#include <thread>

namespace spt::model
{
  struct Configuration
  {
    using Ptr = std::shared_ptr<Configuration>;

    Configuration() = default;
    ~Configuration() = default;
    Configuration( Configuration&& ) = default;
    Configuration& operator=( Configuration&& ) = default;

    Configuration( const Configuration& ) = delete;
    Configuration& operator=( const Configuration& ) = delete;

    std::string key;
    std::string secret;
    std::string region;
    std::string bucket;
    std::string authKey;
    std::string cacheDir{ "/tmp" };
    std::string logLevel{ "info" };
    std::string mmdbHost;
    std::string akumuli;
    std::string metricPrefix{ "request" };
    std::string mongoUri;
    std::string mongoDatabase{ "metrics" };
    std::string mongoCollection{ "request" };
    int port = 8000;
    int mmdbPort = 8010;
    int akumuliPort = 8282;
    int ttl = 300;
    int threads = std::thread::hardware_concurrency();
    bool rejectQueryStrings = false;

    [[nodiscard]] std::string str() const;
  };

  bool publishMetrics( const Configuration& config );
}
