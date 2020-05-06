//
// Created by Rakesh on 06/05/2020.
//

#pragma once

#include <optional>
#include <sstream>

namespace spt::server
{
  struct S3Object
  {
    using Opt = std::optional<S3Object>;

    S3Object() = default;
    ~S3Object() = default;
    S3Object( S3Object&& ) = default;
    S3Object& operator=( S3Object&& ) = default;

    S3Object( const S3Object& ) = delete;
    S3Object& operator=( const S3Object& ) = delete;

    inline std::string str() const
    {
      std::ostringstream ss;
      ss << R"({"fileName": ")" << fileName <<
        R"(", "contentType": ")" << contentType <<
        R"(", "lastModified": ")" << lastModified <<
        R"(", "contentLength": )" << contentLength <<
        R"(, "etag": ")" << etag <<
        R"(", "expires": ")" << expires <<
        R"(", "cacheControl": ")" << cacheControl <<
        "\"}";
      return ss.str();
    }

    std::string contentType;
    std::string fileName;
    std::string etag;
    std::string cacheControl;
    std::string expires;
    std::string lastModified;
    std::size_t contentLength;
  };
}
