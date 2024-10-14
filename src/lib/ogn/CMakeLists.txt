cmake_minimum_required(VERSION 3.5.0)

project(ogn VERSION 0.0.0 LANGUAGES CXX)
set(MSG_PREFIX "${PROJECT_NAME} |")

set(MODULE_SOURCE_FILES
    ace/ognpacket.cpp
    ace/ogn1.cpp
)

set(MODULE_TARGET_LINK
    libcrc
    hardware_pio
    utils
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)
