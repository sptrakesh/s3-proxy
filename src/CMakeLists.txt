file(GLOB_RECURSE s3proxy_SRCS *.cpp *.h)
set(s3proxy_SRCS ${s3proxy_SRCS})
add_executable(s3proxy ${s3proxy_SRCS} )

target_link_libraries(s3proxy
  PRIVATE ${AWSSDK_LINK_LIBRARIES} Boost::boost ${OPENSSL_LIBRARIES}
)

install(TARGETS s3proxy DESTINATION bin)
