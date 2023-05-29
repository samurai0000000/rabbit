# CMakeLists.txt
#
# Copyright (C) 2023, Charles Chiou

cmake_minimum_required(VERSION 3.0)

project(voicerec)

add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-g -O2)

add_executable(voicerec voicerec.c storage.c filter.c)
target_link_libraries(voicerec mosquitto)
