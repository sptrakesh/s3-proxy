//
// Created by Rakesh on 14/05/2020.
//

#pragma once

#include "model/config.h"

#include <unordered_map>

namespace spt::client
{
  namespace mmdb
  {
    struct Worker;
  }

  class MMDBClient
  {
  public:
    static MMDBClient& instance( model::Configuration::Ptr config = nullptr );
    ~MMDBClient();

    MMDBClient( const MMDBClient& ) = delete;
    MMDBClient& operator=( const MMDBClient& ) = delete;

    using Properties = std::unordered_map<std::string, std::string>;
    Properties query( const std::string& ip );

  private:
    explicit MMDBClient( model::Configuration::Ptr config );

    mmdb::Worker* worker = nullptr;
  };
}
