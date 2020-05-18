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

    Metric( const Metric& ) = default;
    Metric& operator=( const Metric& ) = default;

    std::string str() const;

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
