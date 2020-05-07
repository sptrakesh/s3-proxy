//
// Created by Rakesh on 05/05/2020.
//

#include "s3util.h"
#include "log/NanoLog.h"
#include "util/cache.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/model/GetObjectRequest.h>

using spt::server::S3Util;

S3Util& S3Util::instance( util::Configuration::Ptr configuration )
{
  Aws::InitAPI( {} );
  static S3Util instance{ std::move( configuration ) };
  return instance;
}

S3Util::~S3Util()
{
  Aws::ShutdownAPI( {} );
}

S3Util::S3Util( util::Configuration::Ptr config ) :
    configuration{ std::move( config ) }
{
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = configuration->region;
  client = std::make_unique<Aws::S3::S3Client>(
      Aws::Auth::AWSCredentials{ configuration->key, configuration->secret },
      clientConfig );
}

spt::util::S3Object::Ptr S3Util::get( const std::string& name )
{
  try
  {
    auto& cache = util::getMetadataCache();
    const auto iter = cache.find( name );
    if ( iter != cache.end() )
    {
      const auto now = std::chrono::system_clock::now();
      if ( now < iter->second->expires )
      {
        LOG_DEBUG << "Returning cached content for " << name;
        if ( std::filesystem::exists( iter->second->fileName ) ) return iter->second;
        else
        {
          LOG_WARN << "Previously cached file for " << name << " at " << iter->second->fileName << " gone.";
        }
      }
      else
      {
        LOG_DEBUG << "Cached content expired for " << name;
      }
    }

    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket( configuration->bucket );
    request.SetKey( name );

    auto outcome = client->GetObject( request );
    if ( !outcome.IsSuccess() )
    {
      const auto& error = outcome.GetError();
      LOG_WARN << "ERROR: " << error.GetExceptionName() << ": " << error.GetMessage();
      return nullptr;
    }

    std::ostringstream ss;
    ss << configuration->cacheDir << '/' << std::hash<std::string>{}( name );
    const auto fileName = ss.str();

    const auto now = std::chrono::system_clock::now();
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch() );
    std::string target;
    target.reserve( fileName.size() + 16 );
    target.append( fileName );
    target.append( "." );
    target.append( std::to_string( us.count() ) );

    auto result = outcome.GetResultWithOwnership();
    std::ofstream of{ target, std::ios::binary };
    of << result.GetBody().rdbuf();

    std::filesystem::rename( target, fileName );
    auto obj = std::make_shared<util::S3Object>();
    obj->contentType = result.GetContentType();
    obj->fileName = fileName;
    obj->etag = result.GetETag();
    obj->cacheControl = result.GetCacheControl();
    if ( obj->cacheControl.empty() )
    {
      obj->cacheControl = std::string{ "max-age=" }.append( std::to_string( configuration->ttl ) );
    }

    obj->lastModified = result.GetLastModified().UnderlyingTimestamp();
    obj->expires += std::chrono::seconds( configuration->ttl );
    obj->contentLength = result.GetContentLength();

    cache.put( name, obj );
    return obj;
  }
  catch ( const std::exception& e )
  {
    LOG_CRIT << "Error: " << e.what();
  }

  return nullptr;
}

