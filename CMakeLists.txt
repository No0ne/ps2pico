cmake_minimum_required(VERSION 3.13)

# Pull in Raspberry Pi Pico SDK
include(~/pico-sdk/external/pico_sdk_import.cmake)

project(ps2pico)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(ps2pico main.c)

target_include_directories(ps2pico PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(ps2pico pico_stdlib tinyusb_host tinyusb_board)

pico_add_extra_outputs(ps2pico)