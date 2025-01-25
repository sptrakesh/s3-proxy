cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# For INTERPROCEDURAL_OPTIMIZATION
if (POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif()
# For Boost
if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project(s3-proxy)

# Enable C++23 features
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if (APPLE)
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/boost")
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/nghttp2")

  execute_process(
      COMMAND brew --prefix libnghttp2
      RESULT_VARIABLE BREW_NGHTTP2
      OUTPUT_VARIABLE BREW_NGHTTP2_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (BREW_NGHTTP2 EQUAL 0 AND EXISTS "${BREW_NGHTTP2_PREFIX}")
    message(STATUS "Found libnghttp2 keg installed by Homebrew at ${BREW_NGHTTP2_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "${BREW_NGHTTP2_PREFIX}/")
  endif()
endif()

if (UNIX AND NOT APPLE)
  if (CMAKE_HOST_SYSTEM_PROCESSOR EQUAL "arm64" )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
    message(STATUS "Enabling mold linker")
  elseif (CMAKE_HOST_SYSTEM_PROCESSOR EQUAL "aarch64" )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
    message(STATUS "Enabling mold linker")
  endif (CMAKE_HOST_SYSTEM_PROCESSOR EQUAL "arm64" )
endif (UNIX AND NOT APPLE)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_MULTITHREADED ON)
set(OPENSSL_USE_STATIC_LIBS TRUE)

if (APPLE)
  find_package(OpenSSL REQUIRED)
endif()

find_package(Boost REQUIRED COMPONENTS iostreams json url)
find_package(mongocxx REQUIRED)
find_package(fmt REQUIRED)
find_package(range-v3 REQUIRED)
find_package(Threads)
find_package(ZLIB)
find_package(AWSSDK REQUIRED COMPONENTS s3)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
endif()

include_directories(
  ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
)

if (APPLE)
  include_directories(
    /usr/local/aws-sdk/include
    /usr/local/nghttp2/include
    ${BREW_NGHTTP2_PREFIX}/include
  )
  LINK_DIRECTORIES(
    /usr/local/nghttp2/lib
    ${BREW_NGHTTP2_PREFIX}/lib
  )
  set(CMAKE_MACOSX_RPATH 1)
elseif (UNIX)
  link_directories(
    /opt/local/lib
  )
endif()

set(CMAKE_INSTALL_PREFIX /opt/spt)

add_definitions(-DCACHE_TTL=86400)
add_subdirectory(src)

if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(test/integration)
endif()
