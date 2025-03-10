//
// Created by Rakesh on 05/05/2020.
//

#include "s3util.hpp"
#include "log/NanoLog.hpp"
#include "model/config.hpp"
#include "util/cache.hpp"
#include "util/compress.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/s3/model/GetObjectRequest.h>

using spt::server::S3Util;

S3Util& S3Util::instance()
{
  Aws::InitAPI( {} );
  static S3Util instance{};
  return instance;
}

S3Util::~S3Util()
{
  Aws::ShutdownAPI( {} );
}

S3Util::S3Util()
{
  const auto& configuration = model::Configuration::instance();
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = configuration.region;
  client = std::make_unique<Aws::S3::S3Client>(
      Aws::Auth::AWSCredentials{ configuration.key, configuration.secret },
      Aws::MakeShared<Aws::S3::S3EndpointProvider>( Aws::S3::S3Client::GetAllocationTag() ),
      clientConfig );
}

spt::model::S3Object::Ptr S3Util::get( const std::string& name )
{
  try
  {
    const auto& configuration = model::Configuration::instance();
    auto& cache = util::getMetadataCache();
    const auto iter = cache.find( name );
    if ( iter != cache.end() )
    {
      const auto now = std::chrono::system_clock::now();
      if ( now < iter->second->expires )
      {
        LOG_DEBUG << "Returning cached content for " << name;
        if ( iter->second->fileName.empty() ) return iter->second;
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
    request.SetBucket( configuration.bucket );
    request.SetKey( name );

    auto outcome = client->GetObject( request );
    if ( !outcome.IsSuccess() )
    {
      const auto& error = outcome.GetError();
      LOG_WARN << "ERROR: " << error.GetExceptionName() << ": " << error.GetMessage() <<
        ", response code: " << static_cast<int>(error.GetResponseCode());

      if ( Aws::Http::HttpResponseCode::NOT_FOUND == error.GetResponseCode() )
      {
        LOG_INFO << "Caching not-found state till TTL expires for " << name;
        auto obj = std::make_shared<model::S3Object>();
        obj->expires += std::chrono::seconds( configuration.ttl );
        cache.put( name, obj );
        return obj;
      }
      return nullptr;
    }

    std::ostringstream ss;
    ss << configuration.cacheDir << '/' << std::hash<std::string>{}( name );
    const auto fileName = ss.str();

    const auto now = std::chrono::system_clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>( now.time_since_epoch() );
    std::string target;
    target.reserve( fileName.size() + 32 );
    target.append( fileName );
    target.append( "." );
    target.append( std::to_string( ns.count() ) );

    auto result = outcome.GetResultWithOwnership();
    std::ofstream of{ target, std::ios::binary };
    of << result.GetBody().rdbuf();
    of.close();

    const auto tgz = util::compressedFileName( fileName );
    if ( std::filesystem::exists( tgz ) )
    {
      util::compress( target, tgz );
    }

    std::filesystem::rename( target, fileName );

    auto obj = std::make_shared<model::S3Object>();
    obj->contentType = result.GetContentType();
    obj->fileName = fileName;
    obj->etag = result.GetETag();
    obj->cacheControl = result.GetCacheControl();
    if ( obj->cacheControl.empty() )
    {
      obj->cacheControl = std::string{ "max-age=" }.append( std::to_string( configuration.ttl ) );
    }

    obj->lastModified = result.GetLastModified().UnderlyingTimestamp();
    obj->expires += std::chrono::seconds( configuration.ttl );
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

bool S3Util::clear( const std::string& authKey )
{
  const auto& configuration = model::Configuration::instance();
  if ( configuration.authKey != authKey ) return false;
  LOG_INFO << "Clearing cached object metadata (" << util::getMetadataCache().count() << ")";
  util::getMetadataCache().clear();
  return true;
}

