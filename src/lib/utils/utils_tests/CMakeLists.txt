cmake_minimum_required(VERSION 3.18)
project(utils_tests)
include(FetchContent)

message(STATUS "Building tests.")

add_definitions(-DCATCH_CONFIG_NO_POSIX_SIGNALS)
add_definitions(-DUNIT_TESTING)
add_definitions(-DOPENACE_EMAFLOAT_K_FACTOR_1S=1.9f)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


# Pull in the Catch2 framework.
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.1)
FetchContent_MakeAvailable(Catch2)

# All mocks we might need
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/mocks")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../ace")

# Other dependencies
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/core")
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/utils")
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/utils")

# Add cmake modules
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../vendor/etl etlcpp)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../vendor/libcrc libcrc)

# These examples use the standard separate compilation
set( SOURCES_IDIOMATIC_EXAMPLES
   
   # Tests
   utils_test.cpp
)

string( REPLACE ".cpp" "" BASENAMES_IDIOMATIC_EXAMPLES "${SOURCES_IDIOMATIC_EXAMPLES}" )
set( TARGETS_IDIOMATIC_EXAMPLES ${BASENAMES_IDIOMATIC_EXAMPLES} )

set( ACE_SOURCE_FILES
 #    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/ace/messagerouter.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/core/ace/basemodule.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/core/ace/constants.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/core/ace/coreutils.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/utils/ace/utils.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/utils/ace/encryption.cpp
      # ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/utils/ace/ognconv.cpp

      ../ace/encryption.cpp
      ../ace/EMA.cpp
      ../ace/utils.cpp
)


foreach( name ${TARGETS_IDIOMATIC_EXAMPLES} )
    add_executable( ${name}    
    ${ACE_SOURCE_FILES}
      ${name}.cpp )
endforeach()

set(ALL_EXAMPLE_TARGETS
  ${TARGETS_IDIOMATIC_EXAMPLES}
)

foreach( name ${ALL_EXAMPLE_TARGETS} )
    target_link_libraries( 
      ${name} 
      Catch2WithMain 
      etl
      #      core
      )
endforeach()


list(APPEND CATCH_WARNING_TARGETS ${ALL_EXAMPLE_TARGETS})
set(CATCH_WARNING_TARGETS ${CATCH_WARNING_TARGETS} PARENT_SCOPE)

