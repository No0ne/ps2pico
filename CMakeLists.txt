cmake_minimum_required(VERSION 3.13...3.27)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico CACHE STRING "Board type")
#set(PICO_PLATFORM rp2350 CACHE STRING "rp2350")
#set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.1")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.1.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(target_name ps2pico)
set(target_version 3.0)
project(${target_name} C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(${target_name} src/ps2pico.c src/ps2kbd.c)

# print memory usage, enable all warnings
target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage)
target_compile_options(${target_name} PRIVATE -Wall -Wextra)

add_compile_definitions(PICO_PANIC_FUNCTION=reset)
add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

add_compile_definitions(CLOCKIN=14)
add_compile_definitions(CLOCKOUT=15)
add_compile_definitions(DATAOUT=16)
add_compile_definitions(DATAIN=17)

pico_generate_pio_header(${target_name} ${CMAKE_CURRENT_LIST_DIR}/src/ps2phy.pio)

pico_set_program_name(${target_name} ${target_name})
pico_set_program_version(${target_name} ${target_version})

pico_enable_stdio_uart(${target_name} 1)
pico_enable_stdio_usb(${target_name} 0)

target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(${target_name} pico_stdlib hardware_pio tinyusb_host tinyusb_board)

pico_add_extra_outputs(${target_name})
