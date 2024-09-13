cmake_minimum_required(VERSION 3.13...3.27)

find_package(Perl)
if(NOT PERL_FOUND)
  message(FATAL_ERROR "Perl is needed for generating the fsdata.c file")
endif()

set(OPTMIZE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/external/optimizejson.py
                   openace_default_config.json default_config.hpp)
execute_process(
  COMMAND python ${OPTMIZE_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} ECHO_OUTPUT_VARIABLE
                    ECHO_ERROR_VARIABLE)

# All defines go in config.hpp that should not be configurable for the board
# itself. Default configurations could go here if that makes sense
configure_file(config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp @ONLY)
add_definitions(-include ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp)

set(PICO_BOARD pico_w)

# pico_sdk_import.cmake is a single file copied from this SDK note: this must
# happen before project() include(cmake/pico_sdk_import.cmake)
include(/opt/pico/pico-sdk/pico_sdk_init.cmake)
include(cmake/FreeRTOS_Kernel_import.cmake)

project(OpenACE C CXX ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall -fsingle-precision-constant -ffast-math)
add_link_options(-Wl,--print-memory-usage)

# target_compile_definitions(TARGET PRIVATE PICO_DEFAULT_UART_TX_PIN=16
# PICO_DEFAULT_UART_RX_PIN=17 )
pico_sdk_init()

add_executable(${PROJECT_NAME} main.h main.cpp IdleMemory.c)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../vendor vendorLib)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib globalLib)

target_include_directories(OpenACE PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${PROJECT_NAME} 
  PRIVATE FreeRTOS-Kernel-Heap4
          pico_cyw43_arch_lwip_sys_freertos
          pico_stdlib
          hardware_spi
          hardware_rtc
          pioserial

          # Vendor
          GDL90
          minmea
          etl::etl
          ArduinoJson

          # /lib
          core
          tcpclient
          serialadsb
          config
          ubloxm8n
          adsbdecoder
          libmodes
          dump1090client
          wifiservice
          webserver
          GpsDecoder
          rtc
          acespi
          bmp280
          sx1262
          adsl
          flarm
          ogn
          radiotuner
          gdl90service
          gdloverudp
          utils
          aircrafttracker
          )

target_compile_definitions(OpenACE PRIVATE FREE_RTOS_KERNEL_SMP=1
                                           portSUPPORT_SMP=1)

# Disable USB, enable UART
# USB_UART: Set to 1
pico_enable_stdio_usb(OpenACE 0)
pico_enable_stdio_uart(OpenACE 1)

# create map/bin/hex/uf2 file in addition to ELF. -> rp2_common.cmake
pico_add_extra_outputs(OpenACE)
