//
// Created by Rakesh on 17/05/2020.
//

#include "metric.h"

#include <sstream>

std::string spt::model::Metric::str() const
{
  std::ostringstream oss;
  oss << R"({"method": ")" << method <<
    R"(", "resource": ")" << resource <<
    R"(", "mimeType": ")" << mimeType <<
    R"(", "ipaddess": ")" << ipaddress <<
    "\", \"size\": " << size <<
    ", \"status\": " << status <<
    ", \"compressed\": " << std::boolalpha << compressed <<
    '}';
  return oss.str();
}
