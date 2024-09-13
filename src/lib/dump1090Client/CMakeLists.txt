cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "Dump1090CLient |")
project(dump1090client VERSION 0.0.0 LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
    ace/dump1090client.cpp
)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        )

message("## Including ${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(${PROJECT_NAME} 
PRIVATE
    pico_cyw43_arch_lwip_sys_freertos
    pico_stdlib
    FreeRTOS-Kernel-Heap4 

    core
    tcpclient
    etl::etl
)

