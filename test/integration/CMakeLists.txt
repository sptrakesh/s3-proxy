find_package(cpr REQUIRED)
find_package(fmt REQUIRED)
Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
)

FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE test_SRCS *.cpp *.h)
add_executable(unitTest ${test_SRCS} )

target_link_libraries(unitTest
  PRIVATE Catch2WithMain Boost::boost cpr::cpr fmt::fmt
)
