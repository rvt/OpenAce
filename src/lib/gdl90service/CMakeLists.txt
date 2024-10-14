cmake_minimum_required(VERSION 3.5.0)

project(gdl90service VERSION 0.0.0 LANGUAGES CXX)
set(MSG_PREFIX "${PROJECT_NAME} |")

set(MODULE_SOURCE_FILES
    ace/gdl90service.cpp
)

set(MODULE_TARGET_LINK
    GDL90
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)
