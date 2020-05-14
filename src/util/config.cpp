//
// Created by Rakesh on 2019-05-14.
//

#include "config.h"

#include <sstream>

using spt::util::Configuration;

std::string Configuration::str() const
{
  std::ostringstream ss;
  ss << "{\"key\":" << key <<
    ", \"region\":" << region <<
    ", \"bucket\":" << bucket <<
    ", \"cacheDir\":" << cacheDir <<
    ", \"cacheTTL\":" << ttl <<
    ", \"port\":" << port <<
    ", \"threads\":" << threads <<
    R"(, "mmdb": {"host": ")" << mmdbHost <<
    R"(", "port":)" << mmdbPort << '}' <<
    '}';
  return ss.str();
}
