cmake_minimum_required(VERSION 3.18)
project(coreutils_tests)
include(FetchContent)

message(STATUS "Building tests.")

add_definitions(-DCATCH_CONFIG_NO_POSIX_SIGNALS)
add_definitions(-DUNIT_TESTING)
add_definitions(-DOPENACE_MAXIMUM_TCP_CLIENTS=4)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in the Catch2 framework.
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.1)
FetchContent_MakeAvailable(Catch2)

# Add this module
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../ace")

# Add Mocks
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/mocks")

# Add other modules (usually lib or core)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/core")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/utils")

# Add cmake modules
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../vendor/etl etlcpp)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../vendor/libcrc libcrc)

# These examples use the standard separate compilation
set(SOURCES_IDIOMATIC_EXAMPLES # Tests
    coreutils_test.cpp)

string(REPLACE ".cpp" "" BASENAMES_IDIOMATIC_EXAMPLES
               "${SOURCES_IDIOMATIC_EXAMPLES}")
set(TARGETS_IDIOMATIC_EXAMPLES ${BASENAMES_IDIOMATIC_EXAMPLES})

set(ACE_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/core/ace/constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/core/ace/models.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/core/ace/basemodule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/utils/ace/utils.cpp
    ../ace/coreutils.cpp)

foreach(name ${TARGETS_IDIOMATIC_EXAMPLES})
  add_executable(${name} ${ACE_SOURCE_FILES} ${name}.cpp)

  # Run test for each target
  set(UNIT_TEST ${name})
  add_custom_command(
    TARGET ${UNIT_TEST}
    COMMENT "Run tests"
    POST_BUILD
    COMMAND ${UNIT_TEST})
endforeach()

set(ALL_EXAMPLE_TARGETS ${TARGETS_IDIOMATIC_EXAMPLES})

foreach(name ${ALL_EXAMPLE_TARGETS})
  target_link_libraries(${name} PRIVATE Catch2WithMain etl)
endforeach()

list(APPEND CATCH_WARNING_TARGETS ${ALL_EXAMPLE_TARGETS})
set(CATCH_WARNING_TARGETS
    ${CATCH_WARNING_TARGETS}
    PARENT_SCOPE)
