//
// Created by Rakesh on 05/05/2020.
//

#pragma once
#include "util/config.h"
#include "util/s3object.h"

#include <aws/s3/S3Client.h>

namespace spt::server
{
  class S3Util
  {
  public:
    static S3Util& instance( util::Configuration::Ptr configuration = nullptr );
    ~S3Util();

    util::S3Object::Ptr get( const std::string& name );

  private:
    explicit S3Util( util::Configuration::Ptr configuration );

    util::Configuration::Ptr configuration;
    std::unique_ptr<Aws::S3::S3Client> client;
  };
}
