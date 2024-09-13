cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "GpsDecoder |")
project(GpsDecoder VERSION 0.0.0 LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
    ace/GpsDecoder.cpp
)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

        )

message("## Including ${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    FreeRTOS-Kernel-Heap4 
    pico_stdlib
    core
    minmea
    utils
    etl::etl
)

