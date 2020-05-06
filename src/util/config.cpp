//
// Created by Rakesh on 2019-05-14.
//

#include "config.h"

#include <cstdlib>
#include <sstream>
#include <boost/asio/ip/host_name.hpp>

std::string spt::util::hostname()
{
  char* val = std::getenv( "HOST_NODE" );
  return val == nullptr ? boost::asio::ip::host_name() : std::string( val );
}

using spt::util::Configuration;

std::string Configuration::str() const
{
  std::ostringstream ss;
  ss << "{\"key\":" << key <<
    ", \"region\":" << region <<
    ", \"bucket\":" << bucket <<
    ", \"cacheDir\":" << cacheDir <<
    ", \"cacheInMemory\":" << std::boolalpha << cacheInMemory <<
    ", \"cacheTTL\":" << ttl <<
    ", \"port\":" << port <<
    ", \"threads\":" << threads;
  return ss.str();
}
