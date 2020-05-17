//
// Created by Rakesh on 16/05/2020.
//

#pragma once

#include <chrono>
#include <string>

namespace spt::util
{
  struct Metric
  {
    Metric() = default;
    ~Metric() = default;
    Metric( Metric&& ) = default;
    Metric& operator=( Metric&& ) = default;

    Metric( const Metric& ) = delete;
    Metric& operator=( const Metric& ) = delete;

    std::string method;
    std::string resource;
    std::string mimeType;
    std::string ipaddress;
    std::chrono::nanoseconds time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch() );
    std::size_t size;
    int status;
  };
}
