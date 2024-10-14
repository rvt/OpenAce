# # CMakeLists.txt for a specific module (e.g., adsbdecoder)
# cmake_minimum_required(VERSION 3.5.0)

# project(adsbdecoder VERSION 0.0.0 LANGUAGES CXX)

# # Define the source files for this module
# set(MODULE_SOURCE_FILES
#     ace/addresscache.cpp
#     ace/adsbdatacollector.cpp
#     ace/adsbdecoder.cpp
#     ace/cpr.cpp
# )

# # Any additional libraries appart from the defaults
# set(MODULE_TARGET_LINK
#     libmodes
# )


# # Include the common module
# include(${CMAKE_CURRENT_SOURCE_DIR}/../openace_module.cmake)


cmake_minimum_required(VERSION 3.5.0)

set(MSG_PREFIX "ADSBDecoder |")
project(adsbdecoder VERSION 0.0.0 LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" OFF)
option(NO_STL "No STL" OFF)

add_library(${PROJECT_NAME} STATIC
    ace/addresscache.cpp
    ace/adsbdatacollector.cpp
    ace/adsbdecoder.cpp
    ace/cpr.cpp
)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

message("## Including ${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    FreeRTOS-Kernel-Heap4
    pico_stdlib

    core
    libmodes
    etl::etl
    pico_time
)
