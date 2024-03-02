//
// Created by Rakesh on 02/01/2023.
//

#include "builder.h"
#include <fmt/format.h>

using spt::client::Builder;

namespace spt::client::pbuilder
{
  std::string clean( std::string_view view )
  {
    std::string value{};
    value.reserve( view.size() + 16 );

    for ( std::size_t i = 0; i < view.size(); ++i )
    {
      switch ( view[i] )
      {
      case ',':
      {
        value.append( "\\," );
        break;
      }
      case '"':
      {
        value.append( "\\\"" );
        break;
      }
      case '=':
      {
        value.append( "\\=" );
        break;
      }
      case '\n':
      {
        value.append( "\\\n" );
        break;
      }
      case '\r':
      {
        value.append( "\\\r" );
        break;
      }
      case '\\':
      {
        value.append( 2, '\\' );
        break;
      }
      default:
        value.append( 1, view[i] );
      }
    }

    return value;
  }
}

Builder& Builder::startRecord( std::string_view name )
{
  record = Record{ name };
  return *this;
}

Builder& Builder::addTag( std::string_view key, std::string_view v )
{
  if ( !record->tags.empty() ) record->tags.append( "," );
  record->tags.append( fmt::format( "{}={}", key, pbuilder::clean( v ) ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, bool v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( fmt::format( "{}={}", key, v ) );

  return *this;
}

Builder& Builder::addValue( std::string_view key, int32_t v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( fmt::format( "{}={}i", key, v ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, uint32_t v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( fmt::format( "{}={}u", key, v ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, int64_t v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( fmt::format( "{}={}i", key, v ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, uint64_t v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( fmt::format( "{}={}u", key, v ) );
  return *this;
}

Builder& Builder::addValue( std::string_view key, float v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( fmt::format( "{}={}", key, v ) );

  return *this;
}

Builder& Builder::addValue( std::string_view key, double v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( fmt::format( "{}={}", key, v ) );

  return *this;
}

Builder& Builder::addValue( std::string_view key, std::string_view v )
{
  if ( !record->value.empty() ) record->value.append( "," );
  record->value.append( fmt::format( "{}=\"{}\"", key, pbuilder::clean( v ) ) );
  return *this;
}

Builder& Builder::timestamp( std::chrono::nanoseconds v )
{
  record->timestamp = v;
  return *this;
}

Builder& Builder::endRecord()
{
  value.reserve( 128 );
  value.append( fmt::format( "{},{} {} {}\n", record->name, record->tags, record->value, record->timestamp.count() ) );
  return *this;
}

std::string Builder::finish()
{
  return std::move( value );
}
