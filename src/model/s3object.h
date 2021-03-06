//
// Created by Rakesh on 06/05/2020.
//

#pragma once

#include <chrono>
#include <memory>
#include <string>

namespace spt::model
{
  struct S3Object
  {
    using Ptr = std::shared_ptr<S3Object>;

    S3Object() = default;
    ~S3Object() = default;
    S3Object( S3Object&& ) = default;
    S3Object& operator=( S3Object&& ) = default;

    S3Object( const S3Object& ) = delete;
    S3Object& operator=( const S3Object& ) = delete;

    [[nodiscard]] std::string str() const;
    [[nodiscard]] std::string expirationTime() const;
    [[nodiscard]] std::string lastModifiedTime() const;

    std::string contentType;
    std::string fileName;
    std::string etag;
    std::string cacheControl;
    std::chrono::time_point<std::chrono::system_clock> expires = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock> lastModified = std::chrono::system_clock::now();
    std::size_t contentLength;

  private:
    static std::string toWebTime( const std::chrono::time_point<std::chrono::system_clock>& time );
  };
}
