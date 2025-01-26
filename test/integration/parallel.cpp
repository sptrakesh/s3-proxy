//
// Created by Rakesh on 26/01/2025.
//

#include <expected>
#include <format>
#include <future>
#include <catch2/catch_test_macros.hpp>
#include <cpr/cpr.h>

using std::operator""s;

namespace
{
  namespace parallel
  {
    std::expected<std::string, std::string> response( const std::string& uri )
    {
      using O = std::expected<std::string, std::string>;
      auto response = cpr::Get( cpr::Url{ uri },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      if ( response.status_code == 200 ) return O{ std::in_place, response.text };
      return O{ std::unexpect, response.status_line };
    }
  }
}

SCENARIO( "Parallel tests for proxy server" )
{
  GIVEN( "Server running on port 8001" )
  {
    auto baseUrl = "http://localhost:8001"s;

    WHEN( "Retrieve index file in parallel" )
    {
      constexpr auto total = 128;
      const auto uri = std::format( "{}/index.html", baseUrl );

      for ( auto i = 0; i < 16; i++ )
      {
        auto vec = std::vector<std::future<std::expected<std::string, std::string>>>{};
        vec.reserve( total );

        for ( auto j = 0; j < total; j++ )
        {
          vec.push_back( std::async( std::launch::async,
            [&uri] { return parallel::response( uri ); } ) );
        }

        for ( auto& fut : vec )
        {
          const auto resp = fut.get();
          CHECK( resp.has_value() );
          CHECK_FALSE( resp.value().empty() );
        }
      }
    }
  }
}
