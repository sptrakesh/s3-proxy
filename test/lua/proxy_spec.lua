---
--- Created by rakesh.
--- DateTime: 07/05/2020 19:03
---

local log = require "test.lua.log"
local requests = require "requests"

describe("Basic tests for proxy server", function()
  local baseUrl = "http://localhost:8000"
  local etag, lastModified

  setup(function()
    local headers = {["x-forwarded-for"] = "184.105.163.155"}
    local response = requests.get{baseUrl, headers = headers}
    etag = response.headers["etag"]
    assert.is_not_nil(etag)
    log.info("Etag: ", etag)
    assert.is_not_nil(response.headers["expires"])
    lastModified = response.headers["last-modified"]
    assert.is_not_nil(lastModified)
    log.info("Last-Modified: ", lastModified)
    assert.is_not_nil(response.headers["cache-control"])
    assert.is_not_nil(response.headers["content-length"])
  end)

  it("Options request", function()
    local response = requests.options(baseUrl)
    assert.are.equal(204, response.status_code)
    local value = response.headers["access-control-allow-origin"]
    assert.is_not_nil(value)
    log.info("Origin: ", value)
    value = response.headers["access-control-allow-methods"]
    assert.is_not_nil(value)
    log.info("Methods: ", value)
  end)

  it("Retrieve explicit index file", function()
    local headers = {["x-real-ip"] = "184.105.163.155"}
    local response = requests.get{baseUrl..'/index.html', headers = headers }
    assert.are.equal(200, response.status_code)
  end)

  it("Head request for index file", function()
    local response = requests.head(baseUrl)
    assert.is_not_nil(response.headers["etag"])
    assert.is_not_nil(response.headers["expires"])
    assert.is_not_nil(response.headers["last-modified"])
    assert.is_not_nil(response.headers["cache-control"])
    assert.is_not_nil(response.headers["content-length"])
  end)

  it("Get request for index file with If-None-Match", function()
    local headers = {["if-none-match"] = etag, ["x-forwarded-for"] = "184.105.163.155"}
    local response = requests.get{baseUrl, headers = headers, timeout = 2}
    assert.are.equal(304, response.status_code)

    headers = {["If-None-Match"] = etag}
    response = requests.get{baseUrl, headers = headers, timeout = 2}
    assert.are.equal(304, response.status_code)
  end)

  it("Get request for index file with If-Modified-Since", function()
    local headers = {["if-modified-since"] = lastModified, ["x-real-ip"] = "184.105.163.155"}
    local response = requests.get{baseUrl, headers = headers, timeout = 2}
    assert.are.equal(304, response.status_code)

    headers = {["If-Modified-Since"] = lastModified}
    response = requests.get{baseUrl, headers = headers, timeout = 2}
    assert.are.equal(304, response.status_code)
  end)
end)