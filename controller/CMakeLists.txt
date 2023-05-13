# CMakeLists.txt
#
# Copyright (C) 2023, Charles Chiou

cmake_minimum_required(VERSION 3.0)

project(rabbit)

if (CMAKE_SYSTEM_PROCESSOR MATCHES x86_64)
#	set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
#	set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
	set(CMAKE_SYSROOT ${CMAKE_CURRENT_LIST_DIR}/../rpi4rootfs)
	add_link_options(-L ${CMAKE_CURRENT_LIST_DIR}/../rpi4rootfs/usr/lib/aarch64-linux-gnu)
endif ()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")

find_package(OpenCV REQUIRED)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-g -O2)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(../3rdparty/BME280_driver)
include_directories(../3rdparty/wsServer/include)
add_executable(rabbit rabbit.cxx servos.cxx adc.cxx camera.cxx wheels.cxx arms.cxx power.cxx compass.cxx ambience.cxx speech.cxx wifi.cxx medianfilter.cxx keycontrol.cxx websock.cxx logging.cxx crond.cxx ../3rdparty/BME280_driver/bme280.c)
add_subdirectory(../3rdparty/cpp-mjpeg-streamer cpp-mjpeg-streamer)
add_subdirectory(../3rdparty/wsServer wsServer)
target_link_libraries(rabbit pthread bsd pigpio iw asound ${OpenCV_LIBS} nadjieb_mjpeg_streamer::nadjieb_mjpeg_streamer ws)