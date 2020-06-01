//
// Created by Rakesh on 01/06/2020.
//

#include "compress.h"
#include "log/NanoLog.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

std::string spt::util::compressedFileName( const std::string& file )
{
  std::string tgz;
  tgz.reserve( file.size() + 3 );
  tgz.append( file );
  tgz.append( ".gz" );
  return tgz;
}

void spt::util::compress( const std::string& source, const std::string& output )
{
  const auto now = std::chrono::system_clock::now();
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>( now.time_since_epoch() );

  std::string tgz;
  tgz.reserve( source.size() + 32 );
  tgz.append( source );
  tgz.append( std::to_string( ns.count() ) );

  std::ifstream inStream( source, std::ios_base::in | std::ios::binary );
  std::ofstream outStream( tgz, std::ios_base::out | std::ios_base::binary );
  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
  in.push( boost::iostreams::gzip_compressor() );
  in.push( inStream );
  boost::iostreams::copy( in, outStream );

  std::filesystem::rename( tgz, output );
  LOG_DEBUG << "Compressed " << source << " to " << output;
}

