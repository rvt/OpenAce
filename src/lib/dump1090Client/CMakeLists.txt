cmake_minimum_required(VERSION 3.5.0)

project(dump1090client VERSION 0.0.0 LANGUAGES CXX)
set(MSG_PREFIX "${PROJECT_NAME} |")

set(MODULE_SOURCE_FILES
    ace/dump1090client.cpp
)

set(MODULE_TARGET_LINK
    pico_cyw43_arch_lwip_sys_freertos
    tcpclient
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)
