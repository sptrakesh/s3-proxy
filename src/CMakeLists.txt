file(GLOB_RECURSE s3proxy_SRCS *.cpp *.h)
set(s3proxy_SRCS ${s3proxy_SRCS})
add_executable(s3proxy ${s3proxy_SRCS} )

target_compile_definitions(s3proxy
  PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS}
)


if (APPLE)
  target_link_libraries(s3proxy
    PRIVATE ${AWSSDK_LINK_LIBRARIES} ${LIBMONGOCXX_STATIC_LIBRARIES} Boost::boost Boost::iostreams -lnghttp2_asio ${OPENSSL_LIBRARIES} z
  )
else()
  target_link_libraries(s3proxy
    PRIVATE ${AWSSDK_LINK_LIBRARIES} ${LIBMONGOCXX_STATIC_LIBRARIES} Boost::boost Boost::iostreams -lnghttp2 -lnghttp2_asio ${OPENSSL_LIBRARIES} z
  )
endif()

install(TARGETS s3proxy DESTINATION bin)
