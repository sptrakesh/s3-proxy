//
// Created by Rakesh on 19/12/2020.
//

#pragma once

#include "response.hpp"

#include <http2/framework/server.hpp>

namespace spt::server
{
  Response get( const http2::framework::RoutingRequest& rr, const boost::container::flat_map<std::string_view, std::string_view>& params );
  Response clear( const http2::framework::RoutingRequest& rr, const boost::container::flat_map<std::string_view, std::string_view>& params );

  std::string authorise( const http2::framework::Request& req );
  bool shouldCompress( const http2::framework::Request& req );
  std::string ipaddress( const http2::framework::Request& req );
  std::string ifNoneMatch( const http2::framework::Request& req );
  std::string ifModifiedSince( const http2::framework::Request& req );

  using Output = std::tuple<std::string,bool>;
  Output compress( std::string_view data );
}
