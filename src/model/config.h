//
// Created by Rakesh on 2019-05-14.
//

#pragma once

#include <string>
#include <thread>

namespace spt::model
{
  struct Configuration
  {
    static Configuration& instance();

    ~Configuration() = default;
    Configuration( Configuration&& ) = delete;
    Configuration& operator=( Configuration&& ) = delete;

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
    std::string ilpHost;
    std::string ilpSeries{ "request" };
    std::string mongoUri;
    std::string mongoDatabase{ "metrics" };
    std::string mongoCollection{ "request" };
    int port{ 8000 };
    int mmdbPort{ 8010 };
    int ilpPort{ 9009 };
    int ttl{ 300 };
    int threads = std::thread::hardware_concurrency();
    bool rejectQueryStrings{ false };

    [[nodiscard]] std::string str() const;

  private:
    Configuration() = default;
  };

  bool publishMetrics( const Configuration& config );
}
