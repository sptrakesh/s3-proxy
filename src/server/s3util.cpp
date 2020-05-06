//
// Created by Rakesh on 05/05/2020.
//

#include "s3util.h"
#include "log/NanoLog.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <aws/core/Aws.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/model/GetObjectRequest.h>

namespace spt::server::ps3
{
  Aws::SDKOptions options{};
  std::unique_ptr<S3Util> s3util = nullptr;

  std::string lastModified( const Aws::Utils::DateTime& date )
  {
    std::ostringstream oss;

    switch ( date.GetDayOfWeek() )
    {
    case Aws::Utils::DayOfWeek::Sunday:
      oss << "Sun, ";
      break;
    case Aws::Utils::DayOfWeek::Monday:
      oss << "Mon, ";
      break;
    case Aws::Utils::DayOfWeek::Tuesday:
      oss << "Tue, ";
      break;
    case Aws::Utils::DayOfWeek::Wednesday:
      oss << "Wed, ";
      break;
    case Aws::Utils::DayOfWeek::Thursday:
      oss << "Thu, ";
      break;
    case Aws::Utils::DayOfWeek::Friday:
      oss << "Fri, ";
      break;
    case Aws::Utils::DayOfWeek::Saturday:
      oss << "Sat, ";
      break;
    }

    if ( date.GetDay() < 10 ) oss << '0';
    oss << date.GetDay() << ' ';

    switch ( date.GetMonth() )
    {
    case Aws::Utils::Month::January:
      oss << "Jan ";
      break;
    case Aws::Utils::Month::February:
      oss << "Feb ";
      break;
    case Aws::Utils::Month::March:
      oss << "Mar ";
      break;
    case Aws::Utils::Month::April:
      oss << "Apr ";
      break;
    case Aws::Utils::Month::May:
      oss << "May ";
      break;
    case Aws::Utils::Month::June:
      oss << "Jun ";
      break;
    case Aws::Utils::Month::July:
      oss << "Jul ";
      break;
    case Aws::Utils::Month::August:
      oss << "Aug ";
      break;
    case Aws::Utils::Month::September:
      oss << "Sep ";
      break;
    case Aws::Utils::Month::October:
      oss << "Oct ";
      break;
    case Aws::Utils::Month::November:
      oss << "Nov ";
      break;
    case Aws::Utils::Month::December:
      oss << "Dec ";
      break;
    }

    oss << date.GetYear() << ' ';

    if ( date.GetHour() < 10 ) oss << '0';
    oss << date.GetHour() << ':';
    if ( date.GetMinute() < 10 ) oss << '0';
    oss << date.GetMinute() << ':';
    if ( date.GetSecond() < 10 ) oss << '0';
    oss << date.GetSecond();

    oss << " GMT";
    return oss.str();
  }
}

using spt::server::S3Object;
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

S3Object::Opt S3Util::get( const std::string& name )
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
    S3Object obj;
    obj.contentType = result.GetContentType();
    obj.fileName = fileName;
    obj.etag = result.GetETag();
    obj.cacheControl = result.GetCacheControl();
    if ( obj.cacheControl.empty() )
    {
      obj.cacheControl = std::string{ "max-age=" }.append( std::to_string( configuration.ttl ) );
    }

    obj.lastModified = ps3::lastModified( result.GetLastModified() );

    auto expires = std::chrono::system_clock::now();
    expires += std::chrono::seconds ( configuration.ttl );
    obj.expires = ps3::lastModified( expires );

    obj.contentLength = result.GetContentLength();
    return obj;
  }
  catch ( const std::exception& e )
  {
    LOG_CRIT << "Error: " << e.what();
  }

  return std::nullopt;
}

