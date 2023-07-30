cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.1")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(ps2pico C CXX ASM)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(ps2pico ps2pico.c)

add_compile_definitions(DEBUG=0)
add_compile_definitions(CLKIN=14)
add_compile_definitions(CLKOUT=15)
add_compile_definitions(DATOUT=16)
add_compile_definitions(DATIN=17)

pico_generate_pio_header(ps2pico ${CMAKE_CURRENT_LIST_DIR}/ps2device.pio)

pico_set_program_name(ps2pico "ps2pico")
pico_set_program_version(ps2pico "1.1")

pico_enable_stdio_uart(ps2pico 1)
pico_enable_stdio_usb(ps2pico 0)

target_include_directories(ps2pico PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(ps2pico pico_stdlib hardware_pio tinyusb_host tinyusb_board)

pico_add_extra_outputs(ps2pico)
