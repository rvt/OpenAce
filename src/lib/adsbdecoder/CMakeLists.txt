cmake_minimum_required(VERSION 3.5.0)

project(adsbdecoder VERSION 0.0.0 LANGUAGES CXX)
set(MSG_PREFIX "${PROJECT_NAME} |")

set(MODULE_SOURCE_FILES
    ace/addresscache.cpp
    ace/adsbdatacollector.cpp
    ace/adsbdecoder.cpp
    ace/cpr.cpp
)

set(MODULE_TARGET_LINK
    utils
    libmodes
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)
