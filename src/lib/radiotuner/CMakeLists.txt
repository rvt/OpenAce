cmake_minimum_required(VERSION 3.5.0)

project(radiotuner VERSION 0.0.0 LANGUAGES CXX)
set(MSG_PREFIX "${PROJECT_NAME} |")

set(MODULE_SOURCE_FILES
    ace/radiotunerrx.cpp 
    ace/radiotunertx.cpp
    ace/countryregulations.cpp
)

set(MODULE_TARGET_LINK
    pico_time 
    pico_rand
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)

