//
// Created by Rakesh on 26/02/2024.
//
#include <catch2/catch_test_macros.hpp>
#include <cpr/cpr.h>
#include <random>
#include <vector>

using std::operator""s;

SCENARIO( "Access proxy server from multiple IP addresses" )
{
  GIVEN( "Proxy server running on port 8001" )
  {
    auto baseUrl = "http://localhost:8001"s;
    auto ips = std::vector<std::string>{
        "1.32.44.93"s,
        "102.164.199.78"s,
        "102.176.160.70"s,
        "102.176.160.82"s,
        "102.176.160.84"s,
        "103.101.17.171"s,
        "103.101.17.172"s,
        "103.104.119.193"s,
        "103.11.217.165"s,
        "103.110.110.226"s,
        "103.12.196.38"s,
        "103.204.220.24"s,
        "103.21.150.184"s,
        "103.28.121.58"s,
        "103.43.203.225"s,
        "103.46.210.34"s,
        "103.46.225.67"s,
        "103.60.137.2"s,
        "103.81.114.182"s,
        "104.244.75.26"s,
        "104.244.77.254"s,
        "105.179.10.210"s,
        "106.104.12.236"s,
        "106.104.151.142"s,
        "109.117.12.51"s,
        "109.167.40.129"s,
        "109.168.18.50"s,
        "110.36.239.234"s,
        "110.39.187.50"s,
        "110.74.195.215"s,
        "110.93.214.36"s,
        "111.220.90.41"s,
        "112.218.125.140"s,
        "113.255.210.168"s,
        "117.141.114.57"s,
        "117.206.150.8"s,
        "117.53.155.153"s,
        "118.163.13.200"s,
        "118.97.193.204"s,
        "119.110.209.94"s,
        "12.139.101.97"s,
        "12.218.209.130"s,
        "123.200.20.242"s,
        "124.106.57.182"s,
        "124.107.224.215"s,
        "124.41.211.180"s,
        "124.41.243.72"s,
        "125.59.223.27"s,
        "128.199.37.176"s
    };

    WHEN( "Access proxy server with random IP addresses" )
    {
      std::random_device r;
      std::default_random_engine e1( r() );
      std::uniform_int_distribution<int> uniform_dist( 0, static_cast<int>( ips.size() ) - 1 );

      for ( auto i = 0; i < 1000; ++i )
      {
        auto response = cpr::Get( cpr::Url{ baseUrl },
            cpr::HttpVersion{ cpr::HttpVersionCode::VERSION_2_0_PRIOR_KNOWLEDGE },
            cpr::Header{ { "x-forwarded-for", ips[uniform_dist( e1 )] } } );
        REQUIRE( response.status_code == 200 );
      }
    }
  }
}
