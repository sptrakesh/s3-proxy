---
--- Created by rakesh.
--- DateTime: 07/05/2020 21:39
---

local requests = require "requests"

describe("Basic negative tests for proxy server", function()
  local baseUrl = "http://localhost:8000"

  it("GET request for non existent resource", function()
    response = requests.get{baseUrl.."/some/random/resource/that/does/not/exist.pdf", timeout = 2}
    assert.are.equal(404, response.status_code)
  end)

  it("POST request", function()
    local data = {username = "test", password = "test"}
    response = requests.post{baseUrl, data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("PUT request", function()
    local data = {username = "test", password = "test"}
    response = requests.put{baseUrl, data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("PATCH request", function()
    local data = {username = "test", password = "test"}
    response = requests.patch{baseUrl, data = data, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)

  it("DELETE request", function()
    response = requests.delete{baseUrl, timeout = 2}
    assert.are.equal(405, response.status_code)
  end)
end)
