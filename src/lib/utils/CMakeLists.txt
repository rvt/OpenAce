cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "Utils |")
project(utils VERSION 0.0.0 LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
    ace/utils.cpp
    ace/ldpc.cpp
    ace/bitcount.cpp
    ace/encryption.cpp
    ace/EMA.cpp
    ace/ognconv.cpp
    ace/moreutils.cpp
    ace/manchester.cpp
)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>

        )

message("## Including ${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    pico_stdlib
    hardware_pio
    etl::etl
)
