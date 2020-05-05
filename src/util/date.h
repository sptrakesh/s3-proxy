//
// Created by Rakesh on 2019-05-29.
//

#pragma once

#include <sstream>

namespace spt::util
{
  bool isLeapYear( int16_t year )
  {
    bool result = false;

    if ( ( year % 400 ) == 0 ) result = true;
    else if ( ( year % 100 ) == 0 ) result = false;
    else if ( ( year % 4 ) == 0 ) result = true;

    return result;
  }

  int64_t microSeconds( const std::string& date )
  {
    const auto microSecondsPerHour = int64_t( 3600000000 );

    char *end;
    const int16_t year = std::strtol( date.substr( 0, 4 ).c_str(), &end, 10 );
    const int16_t month = std::strtol( date.substr( 5, 2 ).c_str(), &end, 10 );
    const int16_t day = std::strtol( date.substr( 8, 2 ).c_str(), &end, 10 );
    const int16_t hour = std::strtol( date.substr( 11, 2 ).c_str(), &end, 10 );
    const int16_t minute = std::strtol( date.substr( 14, 2 ).c_str(), &end, 10 );
    const int16_t second = std::strtol( date.substr( 17, 2 ).c_str(), &end, 10 );
    const int16_t millis = std::strtol( date.substr( 20, 3 ).c_str(), &end, 10 );
    const int16_t micros = std::strtol( date.substr( 23, 3 ).c_str(), &end, 10 );

    int64_t epoch = micros;
    epoch += millis * int64_t( 1000 );
    epoch += second * int64_t( 1000000 );
    epoch += minute * int64_t( 60000000 );
    epoch += hour * microSecondsPerHour;
    epoch += ( day - 1 ) * 24 * microSecondsPerHour;

    const int8_t isLeap = isLeapYear( year );

    for ( int i = 1; i < month; ++i )
    {
      switch ( i )
      {
      case 2:
        epoch += ( (isLeap) ? 29 : 28 ) * 24 * microSecondsPerHour;
        break;
      case 4:
      case 6:
      case 9:
      case 11:
        epoch += 30 * 24 * microSecondsPerHour;
        break;
      default:
        epoch += 31 * 24 * microSecondsPerHour;
      }
    }

    for ( int i = 1970; i < year; ++i )
    {
      if ( isLeapYear( i ) ) epoch += 366 * 24 * microSecondsPerHour;
      else epoch += 365 * 24 * microSecondsPerHour;
    }

    return epoch;
  }

  std::string isoDate( int64_t epoch )
  {
    const int micros = epoch % int64_t( 1000 );
    epoch /= int64_t( 1000 );

    const int millis = epoch % int64_t( 1000 );
    epoch /= int64_t( 1000 );

    const int second = epoch % 60;

    epoch /= 60;
    const int minute = epoch % 60;

    epoch /= 60;
    const int hour = epoch % 24;
    epoch /= 24;
    int year = 1970;

    {
      int32_t days = 0;
      while ( ( days += ( isLeapYear( year ) ) ? 366 : 365 ) <= epoch ) ++year;

      days -= ( isLeapYear( year ) ) ? 366 : 365;
      epoch -= days;
    }

    uint8_t isLeap = isLeapYear( year );
    int month = 1;

    for ( ; month < 13; ++month )
    {
      int8_t length = 0;

      switch ( month )
      {
      case 2:
        length = isLeap ? 29 : 28;
        break;
      case 4:
      case 6:
      case 9:
      case 11:
        length = 30;
        break;
      default:
        length = 31;
      }

      if ( epoch >= length ) epoch -= length;
      else break;
    }

    const int day = epoch + 1;
    std::stringstream ss;
    ss << year << '-';

    if ( month < 10 ) ss << 0;
    ss << month << '-';

    if ( day < 10 ) ss << 0;
    ss << day << 'T';

    if ( hour < 10 ) ss << 0;
    ss << hour << ':';

    if ( minute < 10 ) ss << 0;
    ss << minute << ':';

    if ( second < 10 ) ss << 0;
    ss << second << '.';

    if ( millis < 10 ) ss << "00";
    else if ( millis < 100 ) ss << 0;
    ss << millis;

    if ( micros < 10 ) ss << "00";
    else if ( micros < 100 ) ss << 0;
    ss << micros << 'Z';

    return ss.str();
  }
}
