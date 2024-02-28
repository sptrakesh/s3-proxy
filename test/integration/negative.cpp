//
// Created by Rakesh on 26/02/2024.
//

#include <catch2/catch_test_macros.hpp>
#include <cpr/cpr.h>
#include <fmt/format.h>

using std::operator""s;

SCENARIO( "Basic negative tests for proxy server" )
{
  GIVEN( "Server running on port 8001" )
  {
    auto baseUrl = "http://localhost:8001"s;

    THEN( "GET request for non existent resource" )
    {
      auto response = cpr::Get( cpr::Url{ fmt::format( "{}/some/random/resource/that/does/not/exist.pdf", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 404 );
    }

    AND_THEN( "GET request with query string does not cause crash" )
    {
      auto response = cpr::Get( cpr::Url{ fmt::format( "{}/index.html?query=blah", baseUrl ) },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 200 );
      CHECK_FALSE( response.header.empty() );
    }

    AND_WHEN( "Making POST request" )
    {
      auto response = cpr::Post( cpr::Url{ fmt::format( "{}/index.html?query=blah", baseUrl ) },
          cpr::Authentication{ "test", "test", cpr::AuthMode::BASIC },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 405 );
    }

    AND_WHEN( "Making PUT request" )
    {
      auto response = cpr::Put( cpr::Url{ fmt::format( "{}/index.html?query=blah", baseUrl ) },
          cpr::Authentication{ "test", "test", cpr::AuthMode::BASIC },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 405 );
    }

    AND_WHEN( "Making PATCH request" )
    {
      auto response = cpr::Patch( cpr::Url{ fmt::format( "{}/index.html?query=blah", baseUrl ) },
          cpr::Authentication{ "test", "test", cpr::AuthMode::BASIC },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 405 );
    }

    AND_WHEN( "Making DELETE request" )
    {
      auto response = cpr::Delete( cpr::Url{ fmt::format( "{}/index.html?query=blah", baseUrl ) },
          cpr::Authentication{ "test", "test", cpr::AuthMode::BASIC },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 405 );
    }

    AND_WHEN( "Making an OPTIONS request with unknown origin" )
    {
      auto response = cpr::Options( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" }, { "origin", "https://184.105.163.15" } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 204 );
      CHECK_FALSE( response.header.contains( "access-control-allow-origin" ) );
      CHECK( response.header.contains( "access-control-allow-methods" ) );
    }
  }
}
