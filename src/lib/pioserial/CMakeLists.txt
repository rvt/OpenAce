cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "PioSerial |")
project(pioserial VERSION 0.0.0 LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
    ace/pioserial.cpp
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/ace/uart_rx.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/ace/uart_tx.pio)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>

        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

message("## Including ${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    FreeRTOS-Kernel-Heap4    
    pico_stdlib

    hardware_gpio
    hardware_irq
    hardware_uart
    hardware_pio
  
    core 
    utils

    etl::etl
)

