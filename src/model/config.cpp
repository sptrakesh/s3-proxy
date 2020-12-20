//
// Created by Rakesh on 2019-05-14.
//

#include "config.h"

#include <sstream>

#include <mongocxx/uri.hpp>

using spt::model::Configuration;

Configuration& Configuration::instance()
{
  static Configuration conf;
  return conf;
}

std::string Configuration::str() const
{
  std::ostringstream moss;
  if ( !mongoUri.empty() )
  {
    auto u = mongocxx::uri{ mongoUri };
    moss << "mongodb://";
    bool first = true;
    for ( const auto& host : u.hosts() )
    {
      if ( !first ) moss << ',';
      moss << host.name << ':' << host.port;
      first = false;
    }

    moss << '/' << u.database();
    const auto pos = mongoUri.find( '?' );
    if ( pos != std::string::npos )
    {
      moss << mongoUri.substr( pos );
    }
  }

  std::ostringstream ss;
  ss << R"({"key": ")" << key <<
    R"(", "region": ")" << region <<
    R"(", "bucket": ")" << bucket <<
    R"(", "cacheDir": ")" << cacheDir <<
    R"(", "cacheTTL": )" << ttl <<
    ", \"port\": " << port <<
    ", \"threads\": " << threads <<
    R"(, "rejectQueryStrings": ")" << std::boolalpha << rejectQueryStrings <<
    R"(, "logLevel": ")" << logLevel <<
    R"(", "mmdb": {"host": ")" << mmdbHost <<
    R"(", "port": )" << mmdbPort << '}' <<
    R"(, "akumuli": {"host": ")" << akumuli <<
    R"(", "prefix": ")" << metricPrefix <<
    R"(", "port": )" << akumuliPort << '}' <<
    R"(, "mongo": {"uri": ")" << moss.str() <<
    R"(", "database": ")" << mongoDatabase <<
    R"(", "collection": ")" << mongoCollection << "\"}" <<
    '}';
  return ss.str();
}

bool spt::model::publishMetrics( const Configuration& config )
{
  return !config.mmdbHost.empty() && (!config.akumuli.empty() || !config.mongoUri.empty());
}
