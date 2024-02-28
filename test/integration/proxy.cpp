//
// Created by Rakesh on 26/02/2024.
//

#include <catch2/catch_test_macros.hpp>
#include <cpr/cpr.h>
#include <fmt/format.h>

using std::operator""s;

SCENARIO( "Basic tests for proxy server" )
{
  GIVEN( "Server running on port 8001" )
  {
    auto baseUrl = "http://localhost:8001"s;

    WHEN( "Making a basic request to server" )
    {
      auto response = cpr::Get( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 200 );
      CHECK( response.header.contains( "etag" ) );
      CHECK( response.header.contains( "expires" ) );
      CHECK( response.header.contains( "last-modified" ) );
      CHECK( response.header.contains( "cache-control" ) );
      CHECK( response.header.contains( "content-length" ) );
    }

    AND_WHEN( "Making an OPTIONS request" )
    {
      auto response = cpr::Options( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" }, { "origin", "http://127.0.0.1:8080" } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 204 );
      CHECK( response.header.contains( "access-control-allow-origin" ) );
      CHECK( response.header.contains( "access-control-allow-methods" ) );
    }

    AND_WHEN( "Retrieve explicit index file" )
    {
      auto response = cpr::Get( cpr::Url{ fmt::format( "{}/index.html", baseUrl ) },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 200 );
    }

    AND_WHEN( "Making HEAD request to server" )
    {
      auto response = cpr::Head( cpr::Url{ baseUrl },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 200 );
      CHECK( response.header.contains( "etag" ) );
      CHECK( response.header.contains( "expires" ) );
      CHECK( response.header.contains( "last-modified" ) );
      CHECK( response.header.contains( "cache-control" ) );
      CHECK( response.header.contains( "content-length" ) );
    }

    AND_WHEN( "Get request for index file with If-None-Match" )
    {
      auto response = cpr::Get( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 200 );
      CHECK( response.header.contains( "etag" ) );
      auto etag = response.header["etag"];

      response = cpr::Get( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" }, { "if-none-match", etag } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 304 );

      response = cpr::Get( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" }, { "If-None-Match", etag } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 304 );
    }

    AND_WHEN( "Get request for index file with If-Modified-Since" )
    {
      auto response = cpr::Get( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 200 );
      CHECK( response.header.contains( "last-modified" ) );
      auto lm = response.header["last-modified"];

      response = cpr::Get( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" }, { "if-modified-since", lm } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 304 );

      response = cpr::Get( cpr::Url{ baseUrl },
          cpr::Header{ { "x-forwarded-for", "184.105.163.155" }, { "If-Modified-Since", lm } },
          cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE } );
      CHECK( response.status_code == 304 );
    }
  }
}
