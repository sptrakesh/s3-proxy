file(GLOB_RECURSE s3proxy_SRCS *.cpp *.h)
set(s3proxy_SRCS ${s3proxy_SRCS})
add_executable(s3proxy ${s3proxy_SRCS} )

target_compile_definitions(s3proxy
  PRIVATE ${LIBMONGOCXX_STATIC_DEFINITIONS}
)

target_link_libraries(s3proxy
  PRIVATE ${AWSSDK_LINK_LIBRARIES} ${LIBMONGOCXX_STATIC_LIBRARIES} Boost::boost Boost::iostreams ${OPENSSL_LIBRARIES} z
)

install(TARGETS s3proxy DESTINATION bin)
