//
// Created by Rakesh on 2019-09-26.
//

#include <cstdlib>
#include <iostream>
#include <mongocxx/instance.hpp>

#include "log/NanoLog.h"
#include "server/server.h"
#include "model/config.h"
#include "util/clara.h"

int main( int argc, char const * const * argv )
{
  using clara::Opt;

  auto& config = spt::model::Configuration::instance();
  bool console = false;
  std::string dir{ "logs/" };
  bool help = false;

  auto options = clara::Help(help) |
      Opt(config.authKey, "authKey")["-a"]["--auth-key"]("Bearer token to use for management API endpoints") |
      Opt(config.key, "key")["-k"]["--key"]("AWS IAM account key to use") |
      Opt(config.secret, "secret")["-s"]["--secret"]("AWS IAM account secret to use") |
      Opt(config.port, "port")["-p"]["--port"]("Port on which to listen (default 8000)") |
      Opt(config.threads, "threads")["-n"]["--threads"]("Number of server threads to spawn (default system)") |
      Opt(config.rejectQueryStrings, "rejectQueryStrings")["-j"]["--reject-query-strings"]("Reject query strings in request (default false)") |
      Opt(console, "console")["-c"]["--console"]("Log to console (default false)") |
      Opt(config.logLevel, "logLevel")["-l"]["--log-level"]("Log level to use [debug|info|warn|critical] (default info).") |
      Opt(config.ttl, "TTL")["-t"]["--ttl"]("TTL for local cache in seconds (default 300)") |
      Opt(config.cacheDir, "cacheDir")["-d"]["--cache-dir"]("Location for local cache (default /tmp)") |
      Opt(config.region, "region")["-r"]["--region"]("AWS region for the account") |
      Opt(config.bucket, "bucket")["-b"]["--bucket"]("AWS bucket from which files are to be served") |
      Opt(config.mmdbHost, "mmdbHost")["-m"]["--mmdb-host"]("MMDB TCP service host. Disabled if not specified.") |
      Opt(config.mmdbPort, "mmdbPort")["-x"]["--mmdb-port"]("MMDB TCP service port (default 2010)") |
      Opt(config.akumuli, "akumuliHost")["-y"]["--akumuli-host"]("Akumuli TSDB host. Disabled if not specified or if MMDB host is not specified.") |
      Opt(config.metricPrefix, "metricPrefix")["-e"]["--akumuli-metric-prefix"]("Akumuli metric prefix (default request)") |
      Opt(config.akumuliPort, "akumuliPort")["-z"]["--akumuli-port"]("Akumuli RESP service port (default 8282)") |
      Opt(config.mongoUri, "mongoUri")["-f"]["--mongo-uri"]("MongoDB connection uri. Disabled if not specified or if MMDB host is not specified.") |
      Opt(config.mongoDatabase, "mongoDatabase")["-g"]["--mongo-database"]("MongoDB database to write metrics to (default metrics)") |
      Opt(config.mongoCollection, "mongoCollection")["-i"]["--mongo-collection"]("MongoDB collection to write metrics to (default request)") |
      Opt(dir, "dir")["-o"]["--dir"]("Log directory (default logs/)");

  auto result = options.parse( clara::Args( argc, argv ) );
  if ( !result )
  {
    std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
    exit( 1 );
  }

  if ( help )
  {
    options.writeToStream(std::cout);
    exit( 0 );
  }

  std::cout << "Starting daemon with options\n" <<
    "configuration: " << config.str() << '\n' <<
    "console: " << std::boolalpha << console << '\n' <<
    "dir: " << dir << '\n';

  if ( config.logLevel == "debug" ) nanolog::set_log_level( nanolog::LogLevel::DEBUG );
  else if ( config.logLevel == "info" ) nanolog::set_log_level( nanolog::LogLevel::INFO );
  else if ( config.logLevel == "warn" ) nanolog::set_log_level( nanolog::LogLevel::WARN );
  else if ( config.logLevel == "critical" ) nanolog::set_log_level( nanolog::LogLevel::CRIT );
  nanolog::initialize( nanolog::GuaranteedLogger(), dir, "s3-proxy", console );
  mongocxx::instance instance{};

  std::set_terminate([](){
    std::cerr << "Unhandled exception encountered" << std::endl;
    std::abort();
  });

  return spt::server::run();
}
