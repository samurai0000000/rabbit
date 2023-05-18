# CMakeLists.txt
#
# Copyright (C) 2023, Charles Chiou

cmake_minimum_required(VERSION 3.10)

include(../3rdparty/pico-sdk/external/pico_sdk_import.cmake)

project(rabbit_mcu C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-g -O2)
add_executable(rabbit_mcu main.c ir.c ultrasound.c led.c)
target_link_libraries(rabbit_mcu pico_stdlib)
pico_add_extra_outputs(rabbit_mcu)
