//
// Created by Rakesh on 2019-05-29.
//

#pragma once

#include <cstdint>
#include <sstream>

namespace spt::util
{
  bool isLeapYear( int16_t year );
  std::string isoDate( int64_t epochInMicroseconds );
}
