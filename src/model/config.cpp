//
// Created by Rakesh on 2019-05-14.
//

#include "config.h"

#include <sstream>

using spt::model::Configuration;

std::string Configuration::str() const
{
  std::ostringstream ss;
  ss << R"({"key": ")" << key <<
    R"(", "region": ")" << region <<
    R"(", "bucket": ")" << bucket <<
    R"(", "cacheDir": ")" << cacheDir <<
    R"(", "cacheTTL": )" << ttl <<
    ", \"port\": " << port <<
    ", \"threads\": " << threads <<
    R"(, "mmdb": {"host": ")" << mmdbHost <<
    R"(", "port": )" << mmdbPort << '}' <<
    R"(, "akumuli": {"host": ")" << akumuli <<
    R"(", "prefix": ")" << metricPrefix <<
    R"(", "port": )" << akumuliPort << '}' <<
    R"(, "mongo": {"uri": ")" << mongoUri <<
    R"(", "database": ")" << mongoDatabase <<
    R"(", "collection": ")" << mongoCollection << "\"}" <<
    '}';
  return ss.str();
}

bool spt::model::publishMetrics( const Configuration& config )
{
  return !config.mmdbHost.empty() && (!config.akumuli.empty() || !config.mongoUri.empty());
}
