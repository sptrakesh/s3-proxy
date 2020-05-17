//
// Created by Rakesh on 2019-05-26.
//

#pragma once

#include <string>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>

namespace wirepulse::akumuli
{
  using Tag = std::pair<std::string, std::string>;
  using Tags = std::vector<Tag>;

  template <typename Value, char ValueType>
  struct TimeSeries
  {
    TimeSeries()
    {
      tags.reserve( 16 );
      timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::system_clock::now().time_since_epoch() );
    }

    TimeSeries( std::string name, Value value ) :
        name{ std::move( name ) }, value{ value }
    {
      tags.reserve( 16 );
      timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::system_clock::now().time_since_epoch() );
    }

    TimeSeries( std::string name, Tags tags, Value value ) :
        name{ std::move( name ) }, tags{ std::move( tags ) }, value{ value }
    {
      timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::system_clock::now().time_since_epoch() );
    }

    TimeSeries( std::string name, Tags tags, std::chrono::nanoseconds timestamp, Value value ) :
        name{ std::move( name ) }, tags{ std::move( tags ) },
        timestamp{ timestamp }, value{ value } {}

    TimeSeries& add( Tag tag )
    {
      tags.emplace_back( std::move( tag ) );
      return *this;
    }

    TimeSeries& add( std::string key, std::string tag )
    {
      tags.emplace_back( std::move( key ), std::move( tag ) );
      return *this;
    }

    [[nodiscard]] std::string resp() const
    {
      std::ostringstream ss;

      if constexpr ( std::is_same_v<Value, std::string> )
      {
        ss << "+!";
      }
      else
      {
        ss << '+';
      }

      ss << name;

      for ( const auto& tag : tags )
      {
        ss << ' ' << tag.first << '=' <<  tag.second;
      }

      ss << "\r\n";

      ss << ':' << timestamp.count() << "\r\n";
      ss << ValueType << value << "\r\n";

      return ss.str();
    }

    TimeSeries(const TimeSeries&) = delete;
    TimeSeries& operator=(const TimeSeries&) = delete;

    TimeSeries(TimeSeries&&) = default;
    TimeSeries& operator=(TimeSeries&&) = default;
    ~TimeSeries() = default;

  private:
    std::string name;
    Tags tags;
    std::chrono::nanoseconds timestamp;
    Value value;
  };

  using IntegerSeries = TimeSeries<int64_t, ':'>;
  using DoubleSeries = TimeSeries<double, '+'>;
  using StringEvent = TimeSeries<std::string, '+'>;

  template <typename V, char VT>
  std::ostream& operator<<( std::ostream& s, const TimeSeries<V, VT>& ts )
  {
    s << ts.resp();
    return s;
  }
}

