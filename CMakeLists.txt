# CMakeLists.txt
#
# Copyright (C) 2023, Charles Chiou

cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")
set(OpenCV_DIR "/usr/lib/arm-linux-gnueabihf/cmake/opencv4")

project(rabbt)
find_package(OpenCV REQUIRED)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-g -O2)
include_directories(${OpenCV_INCLUDE_DIRS})
add_executable(rabbit rabbit.cxx video.cxx)
target_link_libraries(rabbit pthread ${OpenCV_LIBS})
