cmake_minimum_required(VERSION 3.5.0)

project(ubloxm8n VERSION 0.0.0 LANGUAGES CXX)
set(MSG_PREFIX "${PROJECT_NAME} |")

set(MODULE_SOURCE_FILES
    ace/ubloxm8n.cpp
)

set(MODULE_TARGET_LINK
    hardware_pio
    pioserial
    utils
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)
