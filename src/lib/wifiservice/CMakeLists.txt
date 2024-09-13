cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "WifiService |")
project(wifiservice VERSION 0.0.0 LANGUAGES CXX C ASM)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
        ace/wifiservice.cpp
        dnsserver/dnsserver.c
        dhcpserver/dhcpserver.c
)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

target_link_libraries(${PROJECT_NAME} 
PRIVATE
         pico_cyw43_arch_lwip_sys_freertos
         pico_stdlib
         pico_lwip_mdns
         FreeRTOS-Kernel-Heap4 
         core
         etl::etl
)

