cmake_minimum_required(VERSION 3.5.0)

project(core VERSION 0.0.0 LANGUAGES CXX)
set(MSG_PREFIX "${PROJECT_NAME} |")

set(MODULE_SOURCE_FILES
    ace/constants.cpp
    ace/models.cpp
    ace/basemodule.cpp
    ace/coreutils.cpp
)

set(MODULE_TARGET_LINK
    pico_cyw43_arch_lwip_sys_freertos
    ArduinoJson
    hardware_flash
    pico_flash
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)
