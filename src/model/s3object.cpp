//
// Created by Rakesh on 06/05/2020.
//

#include "s3object.h"

#include <sstream>
#include <aws/core/utils/DateTime.h>

using spt::model::S3Object;

std::string S3Object::str() const
{
  std::ostringstream ss;
  ss << R"({"fileName": ")" << fileName <<
     R"(", "contentType": ")" << contentType <<
     R"(", "lastModified": ")" << lastModifiedTime() <<
     R"(", "contentLength": )" << contentLength <<
     R"(, "etag": ")" << etag <<
     R"(", "expires": ")" << expirationTime() <<
     R"(", "cacheControl": ")" << cacheControl <<
     "\"}";
  return ss.str();
}

std::string S3Object::expirationTime() const
{
  return toWebTime( expires );
}

std::string S3Object::lastModifiedTime() const
{
  return toWebTime( lastModified );
}

std::string S3Object::toWebTime( const std::chrono::time_point<std::chrono::system_clock>& time )
{
  std::ostringstream oss;
  const auto date = Aws::Utils::DateTime{ time };

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

