//
// Created by Rakesh on 16/05/2020.
//

#pragma once

#include <chrono>
#include <string>

namespace spt::model
{
  struct Metric
  {
    Metric() = default;
    ~Metric() = default;
    Metric( Metric&& ) = default;
    Metric& operator=( Metric&& ) = default;

    Metric( const Metric& ) = delete;
    Metric& operator=( const Metric& ) = delete;

    [[nodiscard]] std::string str() const;

    std::string method;
    std::string resource;
    std::string mimeType;
    std::string ipaddress;
    std::size_t size;
    std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now();
    int64_t time;
    int status;
    bool compressed;
  };
}
