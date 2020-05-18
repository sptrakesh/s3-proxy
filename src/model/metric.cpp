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
    R"(", "time": )" << time.count() <<
    ", \"size\": " << size <<
    ", \"status\": " << status <<
    '}';
  return oss.str();
}
