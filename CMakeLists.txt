cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.1")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(ps2pico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_compile_definitions(ATPHY)
add_executable(ps2pico src/ps2pico.c src/atphy.c)

##add_compile_definitions(XTPHY)
##add_executable(ps2pico src/ps2pico.c src/xtphy.c)

add_compile_definitions(CLOCKIN=14)
add_compile_definitions(CLOCKOUT=15)
add_compile_definitions(DATAOUT=16)
add_compile_definitions(DATAIN=17)

##add_compile_definitions(XTALT)
##add_executable(ps2pico src/ps2pico.c src/xtphy.c)
##add_compile_definitions(DATAALT=11)
##add_compile_definitions(CLOCKALT=12)
##add_compile_definitions(LVPWR=13)

pico_generate_pio_header(ps2pico ${CMAKE_CURRENT_LIST_DIR}/src/atphy.pio)
pico_generate_pio_header(ps2pico ${CMAKE_CURRENT_LIST_DIR}/src/xtphy.pio)
pico_generate_pio_header(ps2pico ${CMAKE_CURRENT_LIST_DIR}/src/xtalt.pio)

pico_set_program_name(ps2pico "ps2pico")
pico_set_program_version(ps2pico "2.1")

pico_enable_stdio_uart(ps2pico 1)
pico_enable_stdio_usb(ps2pico 0)

target_include_directories(ps2pico PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(ps2pico pico_stdlib hardware_pio tinyusb_host tinyusb_board)

pico_add_extra_outputs(ps2pico)
