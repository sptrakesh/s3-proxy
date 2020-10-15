---
--- Created by rakesh.
--- DateTime: 07/05/2020 21:39
---

local inspect = require "inspect"
local log = require "test.lua.log"
local requests = require "requests"

describe("Basic negative tests for proxy server", function()
  local baseUrl = "http://localhost:8000"

  it("GET request for non existent resource", function()
    local response = requests.get{baseUrl.."/some/random/resource/that/does/not/exist.pdf", timeout = 2}
    assert.are.equal(404, response.status_code)
  end)

  it("GET request with query string does not cause crash", function()
    local response = requests.get{baseUrl.."/index.html?query=blah", timeout = 2}
    assert.is_not_nil(response.headers)
    log.debug(string.format("%s - %s", fn, inspect(response)))
  end)

  it("POST request", function()
    local data = {username = "test", password = "test"}
    local response = requests.post{baseUrl, data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("PUT request", function()
    local data = {username = "test", password = "test"}
    local response = requests.put{baseUrl, data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("PATCH request", function()
    local data = {username = "test", password = "test"}
    local response = requests.patch{baseUrl, data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("DELETE request", function()
    local response = requests.delete{baseUrl, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)
end)
