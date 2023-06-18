# CMakeLists.txt
#
# Copyright (C) 2023, Charles Chiou

cmake_minimum_required(VERSION 3.0)

project(voicerec2)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-g -O2)
include_directories(../3rdparty/whisper.cpp)
link_directories(../build/whisper.cpp)
add_executable(voicerec2 voicerec2.cxx)
target_link_libraries(voicerec2 mosquitto whisper)
