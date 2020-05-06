//
// Created by Rakesh on 05/05/2020.
//

#include "s3util.h"
#include "log/NanoLog.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/Aws.h>
#include <aws/s3/model/GetObjectRequest.h>

namespace spt::server::ps3
{
  Aws::SDKOptions options{};
  std::unique_ptr<S3Util> s3util = nullptr;
}

using spt::server::S3Util;

void S3Util::init( const util::Configuration& configuration )
{
  Aws::InitAPI( ps3::options );
  ps3::s3util = std::unique_ptr<S3Util>( new S3Util( configuration ) );
}

S3Util* S3Util::instance()
{
  return ps3::s3util.get();
}

S3Util::~S3Util()
{
  Aws::ShutdownAPI( ps3::options );
}

S3Util::S3Util( const spt::util::Configuration& configuration ) :
    configuration{ configuration }
{
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = configuration.region;
  client = std::make_unique<Aws::S3::S3Client>(
      Aws::Auth::AWSCredentials{ configuration.key, configuration.secret },
      clientConfig );
}

std::optional<std::string> spt::server::S3Util::get( const std::string& name )
{
  try
  {
    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket( configuration.bucket );
    request.SetKey( name );

    auto outcome = client->GetObject( request );
    if ( !outcome.IsSuccess() )
    {
      const auto& error = outcome.GetError();
      LOG_WARN << "ERROR: " << error.GetExceptionName() << ": " << error.GetMessage();
      return std::nullopt;
    }

    std::ostringstream ss;
    ss << configuration.cacheDir << '/' << std::hash<std::string>{}( name );
    const auto fileName = ss.str();

    auto target = fileName;
    target.append( ".tmp" );

    const auto& body = outcome.GetResultWithOwnership().GetBody();
    std::ofstream of{ target, std::ios::binary };
    of << body.rdbuf();

    std::filesystem::rename( target, fileName );
    return fileName;
  }
  catch ( const std::exception& e )
  {
    LOG_CRIT << "Error: " << e.what();
  }

  return std::nullopt;
}

