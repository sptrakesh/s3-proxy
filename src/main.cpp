//
// Created by Rakesh on 2019-09-26.
//

#include <iostream>

#include "log/NanoLog.h"
#include "server/server.h"
#include "util/clara.h"
#include "util/config.h"

int main( int argc, char const * const * argv )
{
  using clara::Opt;

  auto config = std::make_shared<spt::util::Configuration>();
  bool console = false;
  std::string dir{"logs/"};
  bool help = false;

  auto options = clara::Help(help) |
      Opt(config->authKey, "authKey")["-a"]["--auth-key"]("Bearer token to use for management API endpoints") |
      Opt(config->key, "key")["-k"]["--key"]("AWS IAM account key to use") |
      Opt(config->secret, "secret")["-s"]["--secret"]("AWS IAM account secret to use") |
      Opt(config->port, "port")["-p"]["--port"]("Port on which to listen (default 8000)") |
      Opt(config->threads, "threads")["-n"]["--threads"]("Number of server threads to spawn (default system)") |
      Opt(console, "console")["-c"]["--console"]("Log to console (default false)") |
      Opt(config->ttl, "TTL")["-t"]["--ttl"]("TTL for local cache in seconds (default 300)") |
      Opt(config->cacheDir, "cacheDir")["-d"]["--cache-dir"]("Location for local cache (default /tmp)") |
      Opt(config->region, "region")["-r"]["--region"]("AWS region for the account") |
      Opt(config->bucket, "bucket")["-b"]["--bucket"]("AWS bucket from which files are to be served") |
      Opt(config->mmdbHost, "mmdbHost")["-m"]["--mmdb-host"]("MMDB WebSocket service host. Disabled if not specified.") |
      Opt(config->mmdbPort, "mmdbPort")["-x"]["--mmdb-port"]("MMDB WebSocket service port (default 8010)") |
      Opt(dir, "dir")["-o"]["--dir"]("Log directory (default logs/)");

  auto result = options.parse(clara::Args(argc, argv));
  if (!result)
  {
    std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
    exit(1);
  }

  if (help)
  {
    options.writeToStream(std::cout);
    exit(0);
  }

  std::cout << "Starting daemon with options\n" <<
    "configuration: " << config->str() << '\n' <<
    "console: " << std::boolalpha << console << '\n' <<
    "dir: " << dir << '\n';

  nanolog::initialize( nanolog::GuaranteedLogger(), dir, "s3-proxy", console );

  return spt::server::run( config );
}
