# CMakeLists.txt
#
# Copyright (C) 2023, Charles Chiou

cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")
set(OpenCV_DIR "/usr/lib/arm-linux-gnueabihf/cmake/opencv4")

project(rabbit)
find_package(OpenCV REQUIRED)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-g -O2)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(BME280_driver)
add_executable(rabbit rabbit.cxx servos.cxx adc.cxx camera.cxx wheels.cxx power.cxx compass.cxx ambience.cxx wifi.cxx BME280_driver/bme280.c)
add_subdirectory(cpp-mjpeg-streamer)
target_link_libraries(rabbit pthread bsd pigpio iw ${OpenCV_LIBS} nadjieb_mjpeg_streamer::nadjieb_mjpeg_streamer)
