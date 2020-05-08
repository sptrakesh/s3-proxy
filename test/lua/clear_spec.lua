---
--- Created by rakesh.
--- DateTime: 08/05/2020 06:22
---

local requests = require "requests"

describe("Clear cache tests for proxy server", function()
  local baseUrl = "http://localhost:8000"
  local authKey = "abc123"

  it("GET request to clear cache without authorization header", function()
    local response = requests.get{baseUrl.."/_proxy/_private/_cache/clear", timeout = 2}
    assert.are.equal(403, response.status_code)
  end)

  it("GET request to clear cache with invalid authorization header", function()
    local headers = {["Authorization"] = "blah"}
    local response = requests.get{baseUrl.."/_proxy/_private/_cache/clear", headers = headers, timeout = 2}
    assert.are.equal(400, response.status_code)
  end)

  it("GET request to clear cache with invalid bearer token", function()
    local headers = {["Authorization"] = "Bearer blah"}
    local response = requests.get{baseUrl.."/_proxy/_private/_cache/clear", headers = headers, timeout = 2}
    assert.are.equal(403, response.status_code)
  end)

  it("GET request to clear cache with valid bearer token", function()
    local headers = {["Authorization"] = "Bearer "..authKey}
    local response = requests.get{baseUrl.."/_proxy/_private/_cache/clear", headers = headers, timeout = 2}
    assert.are.equal(304, response.status_code)

    headers = {["authorization"] = "Bearer "..authKey}
    response = requests.get{baseUrl.."/_proxy/_private/_cache/clear", headers = headers, timeout = 2}
    assert.are.equal(304, response.status_code)
  end)

  it("POST request to clear cache", function()
    local data = {username = "test", password = "test"}
    local response = requests.post{baseUrl.."/_proxy/_private/_cache/clear", data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("PUT request to clear cache", function()
    local data = {username = "test", password = "test"}
    local response = requests.put{baseUrl.."/_proxy/_private/_cache/clear", data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("PATCH request to clear cache", function()
    local data = {username = "test", password = "test"}
    local response = requests.patch{baseUrl.."/_proxy/_private/_cache/clear", data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("DELETE request to clear cache", function()
    local response = requests.delete{baseUrl.."/_proxy/_private/_cache/clear", timeout = 2}
    assert.are.equal(405, response.status_code)
  end)
end)
