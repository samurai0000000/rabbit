# CMakeLists.txt
#
# Copyright (C) 2023, Charles Chiou

cmake_minimum_required(VERSION 3.0)

project(rabbit)

if (CMAKE_SYSTEM_PROCESSOR MATCHES x86_64)
#	set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
#	set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
	set(OpenCV_DIR "${CMAKE_CURRENT_LIST_DIR}/rpi4rootfs/usr/lib/aarch64-linux-gnu/cmake/opencv4")
#	add_compile_options(-isystem ${CMAKE_CURRENT_LIST_DIR}/rpi4rootfs/usr/include)
	set(CMAKE_SYSROOT ${CMAKE_CURRENT_LIST_DIR}/rpi4rootfs)
	add_link_options(-L ${CMAKE_CURRENT_LIST_DIR}/rpi4rootfs/usr/lib/aarch64-linux-gnu)
	find_package(Threads REQUIRED)
endif ()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")

find_package(OpenCV REQUIRED)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-g -O2)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(BME280_driver)
add_executable(rabbit rabbit.cxx servos.cxx adc.cxx camera.cxx wheels.cxx arms.cxx power.cxx compass.cxx ambience.cxx wifi.cxx BME280_driver/bme280.c medianfilter.cxx keycontrol.cxx websock.c)
add_subdirectory(cpp-mjpeg-streamer)
add_subdirectory(wsServer)
include_directories(wsServer/include)
target_link_libraries(rabbit pthread bsd pigpio iw ${OpenCV_LIBS} nadjieb_mjpeg_streamer::nadjieb_mjpeg_streamer ws)
