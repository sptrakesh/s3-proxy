file(GLOB_RECURSE s3proxy_SRCS *.cpp *.h)
set(s3proxy_SRCS ${s3proxy_SRCS})
add_executable(s3proxy ${s3proxy_SRCS} )

target_compile_definitions(s3proxy
  PRIVATE _FORTIFY_SOURCE=2
)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Debug build, not enabling IPO/LTO")
else()
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output LANGUAGES CXX)
  if (result)
    message(STATUS "IPO is supported: ${output}")
    set_property(TARGET s3proxy PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized -flto=auto")
  else()
    message(STATUS "IPO is not supported: ${output}")
  endif()
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if (APPLE)
  target_link_libraries(s3proxy
    PRIVATE ${AWSSDK_LINK_LIBRARIES} mongo::mongocxx_static Boost::boost Boost::iostreams Boost::json fmt::fmt range-v3::range-v3 nghttp2_asio ${OPENSSL_LIBRARIES} ZLIB::ZLIB
  )
else()
  target_link_libraries(s3proxy
    PRIVATE ${AWSSDK_LINK_LIBRARIES} mongo::mongocxx_static Boost::boost Boost::iostreams Boost::json fmt::fmt range-v3::range-v3 nghttp2 nghttp2_asio ${OPENSSL_LIBRARIES} ZLIB::ZLIB
  )
endif()

install(TARGETS s3proxy DESTINATION bin)
