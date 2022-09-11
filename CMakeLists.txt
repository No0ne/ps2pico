cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

project(ps2pico C CXX ASM)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(ps2pico main.c)

pico_set_program_name(ps2pico "ps2pico")
pico_set_program_version(ps2pico "0.3")

target_include_directories(ps2pico PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(ps2pico pico_stdlib tinyusb_host tinyusb_board)

pico_add_extra_outputs(ps2pico)