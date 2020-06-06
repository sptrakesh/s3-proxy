---
--- Created by rakesh.
--- DateTime: 05/06/2020 17:29
---

local log = require "test.lua.log"
local requests = require "requests"

describe("Access proxy server from multiple IP addresses", function()
  local baseUrl = "http://localhost:8000"
  local ips = {
  "1.32.44.93",
  "102.164.199.78",
  "102.176.160.70",
  "102.176.160.82",
  "102.176.160.84",
  "103.101.17.171",
  "103.101.17.172",
  "103.104.119.193",
  "103.11.217.165",
  "103.110.110.226",
  "103.12.196.38",
  "103.204.220.24",
  "103.21.150.184",
  "103.28.121.58",
  "103.43.203.225",
  "103.46.210.34",
  "103.46.225.67",
  "103.60.137.2",
  "103.81.114.182",
  "104.244.75.26",
  "104.244.77.254",
  "105.179.10.210",
  "106.104.12.236",
  "106.104.151.142",
  "109.117.12.51",
  "109.167.40.129",
  "109.168.18.50",
  "110.36.239.234",
  "110.39.187.50",
  "110.74.195.215",
  "110.93.214.36",
  "111.220.90.41",
  "112.218.125.140",
  "113.255.210.168",
  "117.141.114.57",
  "117.206.150.8",
  "117.53.155.153",
  "118.163.13.200",
  "118.97.193.204",
  "119.110.209.94",
  "12.139.101.97",
  "12.218.209.130",
  "123.200.20.242",
  "124.106.57.182",
  "124.107.224.215",
  "124.41.211.180",
  "124.41.243.72",
  "125.59.223.27",
  "128.199.37.176"
  };

  it("Access proxy server with random IP addresses", function()
    for _ = 1, 1000
    do
      local ip = ips[ math.random( #ips ) ]
      log.info("Requesting as IP address: ", ip)
      local headers = {["x-forwarded-for"] = ip}
      local response = requests.get{baseUrl, headers = headers}
      assert.are.equal(200, response.status_code)
    end
  end)

end)
