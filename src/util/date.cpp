//
// Created by Rakesh on 19/12/2019.
//

#include "date.h"

bool spt::util::isLeapYear( int16_t year )
{
  bool result = false;

  if ( ( year % 400 ) == 0 ) result = true;
  else if ( ( year % 100 ) == 0 ) result = false;
  else if ( ( year % 4 ) == 0 ) result = true;

  return result;
}

std::string spt::util::isoDate( int64_t epoch )
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
