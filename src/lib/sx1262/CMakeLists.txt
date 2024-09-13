cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "SX1262 |")
project(sx1262 VERSION 0.0.0 LANGUAGES CXX C)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
    ace/sx1262.cpp
    ace/sx1262_hal.cpp

# Driver from Semtech
    driver/src/sx126x.c 
    driver/src/sx126x_lr_fhss.c
    driver/src/sx126x_driver_version.c
    driver/src/lr_fhss_mac.c
)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>

        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        )

message("## Including ${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    FreeRTOS-Kernel-Heap4 
    pico_stdlib
    hardware_spi
    hardware_pio
    
    core
    etl::etl
    utils
)

