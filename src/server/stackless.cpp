//
// Created by Rakesh on 05/05/2020.
//

#include "stackless.h"
#include "log/NanoLog.h"

beast::string_view spt::server::mime_type( beast::string_view path )
{
  using beast::iequals;
  auto const ext = [&path]
  {
    auto const pos = path.rfind( "." );
    if ( pos == beast::string_view::npos )
      return beast::string_view{};
    return path.substr( pos );
  }();
  if ( iequals( ext, ".htm" )) return "text/html";
  if ( iequals( ext, ".html" )) return "text/html";
  if ( iequals( ext, ".php" )) return "text/html";
  if ( iequals( ext, ".css" )) return "text/css";
  if ( iequals( ext, ".txt" )) return "text/plain";
  if ( iequals( ext, ".js" )) return "application/javascript";
  if ( iequals( ext, ".json" )) return "application/json";
  if ( iequals( ext, ".xml" )) return "application/xml";
  if ( iequals( ext, ".swf" )) return "application/x-shockwave-flash";
  if ( iequals( ext, ".flv" )) return "video/x-flv";
  if ( iequals( ext, ".png" )) return "image/png";
  if ( iequals( ext, ".jpe" )) return "image/jpeg";
  if ( iequals( ext, ".jpeg" )) return "image/jpeg";
  if ( iequals( ext, ".jpg" )) return "image/jpeg";
  if ( iequals( ext, ".gif" )) return "image/gif";
  if ( iequals( ext, ".bmp" )) return "image/bmp";
  if ( iequals( ext, ".ico" )) return "image/vnd.microsoft.icon";
  if ( iequals( ext, ".tiff" )) return "image/tiff";
  if ( iequals( ext, ".tif" )) return "image/tiff";
  if ( iequals( ext, ".svg" )) return "image/svg+xml";
  if ( iequals( ext, ".svgz" )) return "image/svg+xml";
  if ( iequals( ext, ".docx" ))
    return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
  if ( iequals( ext, ".xlsx" ))
    return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
  if ( iequals( ext, ".pptx" ))
    return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
  if ( iequals( ext, ".kar" )) return "audio/midi";
  if ( iequals( ext, ".midi" )) return "audio/midi";
  if ( iequals( ext, ".mid" )) return "audio/midi";
  if ( iequals( ext, ".mp3" )) return "audio/mpeg";
  if ( iequals( ext, ".ogg" )) return "audio/ogg";
  if ( iequals( ext, ".m4a" )) return "audio/x-m4a";
  if ( iequals( ext, ".ra" )) return "audio/x-realaudio";
  if ( iequals( ext, ".3gp" )) return "video/3gpp";
  if ( iequals( ext, ".3gpp" )) return "video/3gpp";
  if ( iequals( ext, ".ts" )) return "video/mp2t";
  if ( iequals( ext, ".mp4" )) return "video/mp4";
  if ( iequals( ext, ".mpg" )) return "video/mpeg";
  if ( iequals( ext, ".mpeg" )) return "video/mpeg";
  if ( iequals( ext, ".mov" )) return "video/quicktime";
  if ( iequals( ext, "webm" )) return "video/webm";
  if ( iequals( ext, "flv" )) return "video/x-flv";
  if ( iequals( ext, "m4v" )) return "video/x-m4v";
  if ( iequals( ext, "mng" )) return "video/x-mng";
  if ( iequals( ext, "asf" )) return "video/x-ms-asf";
  if ( iequals( ext, "asx" )) return "video/x-ms-asf";
  if ( iequals( ext, "wmv" )) return "video/x-ms-wmv";
  if ( iequals( ext, "avi" )) return "video/x-msvideo";
  return "application/text";
}

std::string
spt::server::path_cat( beast::string_view base, beast::string_view path )
{
  if ( base.empty())
    return std::string( path );
  std::string result( base );
#ifdef BOOST_MSVC
  char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
  char constexpr path_separator = '/';
  if ( result.back() == path_separator )
    result.resize( result.size() - 1 );
  result.append( path.data(), path.size());
#endif
  return result;
}

void spt::server::fail( beast::error_code ec, char const* what )
{
  LOG_WARN << what << ": " << ec.message();
}
