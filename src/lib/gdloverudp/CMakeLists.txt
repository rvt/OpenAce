cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "Gdl90 |")
project(gdloverudp VERSION 0.0.0 LANGUAGES CXX C)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
    ace/gdloverudp.cpp
)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>

    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
message("## Including ${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    FreeRTOS-Kernel-Heap4
    pico_cyw43_arch_lwip_sys_freertos
    pico_stdlib

    core
    etl::etl
)

