# CMakeLists.txt
#
# Copyright (C) 2023, Charles Chiou

cmake_minimum_required(VERSION 3.0)

project(rabbt)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-g -O2)
add_executable(rabbit rabbit.c)
