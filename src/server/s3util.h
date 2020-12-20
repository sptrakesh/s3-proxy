//
// Created by Rakesh on 05/05/2020.
//

#pragma once

#include "model/s3object.h"

#include <aws/s3/S3Client.h>

namespace spt::server
{
  class S3Util
  {
  public:
    static S3Util& instance();
    ~S3Util();

    model::S3Object::Ptr get( const std::string& name );
    bool clear( const std::string& authKey );

  private:
    S3Util();

    std::unique_ptr<Aws::S3::S3Client> client;
  };
}
