//
// Created by Rakesh on 16/05/2020.
//

#pragma once

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

    std::string str() const;

    std::string method;
    std::string resource;
    std::string mimeType;
    std::string ipaddress;
    std::size_t size;
    int64_t time;
    int status;
    bool compressed;
  };
}
