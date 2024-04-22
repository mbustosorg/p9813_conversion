cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)
project(p9813_conversion C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wno-maybe-uninitialized)
endif()

add_executable(p9813_conversion)

pico_generate_pio_header(p9813_conversion ${CMAKE_CURRENT_LIST_DIR}/clocked_input.pio)
pico_generate_pio_header(p9813_conversion ${CMAKE_CURRENT_LIST_DIR}/p9813.pio)

target_sources(p9813_conversion PRIVATE p9813_conversion.c)

target_link_libraries(p9813_conversion PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        )

pico_add_extra_outputs(p9813_conversion)

pico_enable_stdio_usb(p9813_conversion 1)
