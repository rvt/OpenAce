cmake_minimum_required(VERSION 3.5.0)

project(acespi VERSION 0.0.0 LANGUAGES CXX)
set(MSG_PREFIX "${PROJECT_NAME} |")

set(MODULE_SOURCE_FILES
    ace/acespi.cpp
)

set(MODULE_TARGET_LINK
    hardware_spi
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)


# cmake_minimum_required(VERSION 3.5.0)

# set(MSG_PREFIX "AceSPI |")
# project(acespi VERSION 0.0.0 LANGUAGES CXX C)

# option(BUILD_TESTS "Build unit tests" OFF)
# option(NO_STL "No STL" OFF)

# add_library(${PROJECT_NAME} STATIC
#     ace/acespi.cpp
# )

# target_include_directories(${PROJECT_NAME} INTERFACE
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>

#         PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
#         )

# message("## Including ${CMAKE_CURRENT_LIST_DIR}")

# target_link_libraries(${PROJECT_NAME}
# PRIVATE
#     FreeRTOS-Kernel-Heap4
#     pico_stdlib
#     hardware_spi

#     core
#     etl::etl
# )


