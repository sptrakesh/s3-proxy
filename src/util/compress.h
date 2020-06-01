//
// Created by Rakesh on 01/06/2020.
//

#pragma once

#include <string>

namespace spt::util
{
  std::string compressedFileName( const std::string& file );
  void compress( const std::string& sourceFile, const std::string& outFile );
}
