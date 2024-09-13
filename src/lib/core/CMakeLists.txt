cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "Core |")
project(core VERSION 0.0.0 LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
    ace/constants.cpp
    ace/models.cpp
    ace/basemodule.cpp
    ace/coreutils.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

message("## Including ${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(${PROJECT_NAME}     
    PRIVATE
    pico_stdlib
    FreeRTOS-Kernel-Heap4
#    pico_cyw43_arch_lwip_sys_freertos

    #pico_time
    etl::etl
)
