//
// Created by Rakesh on 05/05/2020.
//

#pragma once
#include "s3object.h"
#include "util/config.h"

#include <aws/s3/S3Client.h>

namespace spt::server
{
  class S3Util
  {
  public:
    static void init( const util::Configuration& configuration );
    static S3Util* instance();
    ~S3Util();

    S3Object::Opt get( const std::string& name );

  private:
    explicit S3Util( const util::Configuration& configuration );

    const util::Configuration& configuration;
    std::unique_ptr<Aws::S3::S3Client> client;
  };
}
