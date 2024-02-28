//
// Created by Rakesh on 26/02/2024.
//

#include <catch2/catch_test_macros.hpp>
#include <cpr/cpr.h>
#include <fmt/format.h>

using std::operator""s;

SCENARIO( "Clear cache tests for proxy server" )
{
  GIVEN( "A local server running on port 8001" )
  {
    auto baseUrl = "http://localhost:8001"s;
    auto authKey = "abc123"s;

    WHEN( "GET request to clear cache without authorization header" )
    {
      auto response = cpr::Get( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 403 );
    }

    AND_WHEN( "GET request to clear cache with invalid authorization header" )
    {
      auto response = cpr::Get( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
          cpr::Header{ { "Authorization", "blah" } } );
      CHECK( response.status_code == 400 );
    }

    AND_WHEN( "GET request to clear cache with invalid bearer token" )
    {
      auto response = cpr::Get( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
          cpr::Bearer{ "blah" } );
      CHECK( response.status_code == 403 );
    }

    AND_WHEN( "GET request to clear cache with valid bearer token" )
    {
      auto response = cpr::Get( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
          cpr::Bearer{ authKey } );
      CHECK( response.status_code == 304 );

      response = cpr::Get( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
          cpr::Bearer{ authKey } );
      CHECK( response.status_code == 304 );
    }

    AND_WHEN( "POST request to clear cache" )
    {
      auto response = cpr::Post( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
          cpr::Bearer{ "blah" } );
      CHECK( response.status_code == 405 );
    }

    AND_WHEN( "PUT request to clear cache" )
    {
      auto response = cpr::Put( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
          cpr::Bearer{ "blah" } );
      CHECK( response.status_code == 405 );
    }

    AND_WHEN( "PATCH request to clear cache" )
    {
      auto response = cpr::Patch( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
          cpr::Bearer{ "blah" } );
      CHECK( response.status_code == 405 );
    }

    AND_WHEN( "DELETE request to clear cache" )
    {
      auto response = cpr::Delete( cpr::Url{ fmt::format( "{}/_proxy/_private/_cache/clear", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
          cpr::Bearer{ "blah" } );
      CHECK( response.status_code == 405 );
    }
  }
}