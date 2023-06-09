# Makefile
#
# Copyright (C) 2023, Charles Chiou

ARCH :=		$(shell uname -m)
MAKEFLAGS =	--no-print-dir
RPI_HOST ?=	rabbit

TARGETS =	build/$(ARCH)/rabbit

ifeq ($(ARCH),x86_64)
PICO_SDK =	$(realpath 3rdparty/pico-sdk)
TARGETS +=	build/$(ARCH)/librealsense/librealsense2.so \
		build/pico/rabbit_mcu.uf2 \
		build/voicerec/voicerec \
		build/voicerec2/voicerec2 \
		build/ros/rabbit

ifeq ($(RPI_HOST),coyote)
ARCH_ENVVARS =	CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++
ROOTFS =	$(realpath rpi2rootfs)
CMAKE_EXTRAS =	-DCMAKE_SYSROOT=$(ROOTFS)
else
ARCH_ENVVARS =	CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++
ROOTFS =	$(realpath rpi4rootfs)
CMAKE_EXTRAS =	-DCMAKE_SYSROOT=$(ROOTFS)
endif
endif

.PHONY: default clean distclean $(TARGETS)

default: $(TARGETS)

clean:
	@test -f build/$(ARCH)/Makefile && $(MAKE) -C build/$(ARCH) clean

distclean:
	rm -rf build

#
# Built the rabbit'bot controller
#

.PHONY: controller

controller: build/$(ARCH)/rabbit

build/$(ARCH)/rabbit: build/$(ARCH)/Makefile build/$(ARCH)/librealsense/librealsense2.so
	@$(MAKE) -C build/$(ARCH)

build/$(ARCH)/Makefile: controller/CMakeLists.txt
	@mkdir -p build/$(ARCH)
	@cd build/$(ARCH) && $(ARCH_ENVVARS) cmake ../../controller

.PHONY: run install install-html

run: build/aarch64/rabbit
	@if [ `hostname` = "$(RPI_HOST)" ]; then \
		sudo killall rabbit >/dev/null 2>&1; \
		sudo ./build/aarch64/rabbit; \
	fi

install: build/$(ARCH)/rabbit
	@if [ `hostname` = "$(RPI_HOST)" ]; then \
		sudo killall rabbit >/dev/null 2>&1; \
		sudo install -m 755 $< /usr/local/bin; \
	else \
		ssh root@$(RPI_HOST) killall rabbit 2>/dev/null && sleep 2; \
		scp $< root@$(RPI_HOST):/usr/local/bin/rabbit ; \
		rsync -avh html root@$(RPI_HOST):/var/www; \
	fi

install-html:
	@rsync -avh html root@$(RPI_HOST):/var/www

#
# Build rabbit'bot ROS nodes
#

ifneq ($(findstring build/ros/rabbit,$(TARGETS)),)

.PHONY: ros

ros: build/ros/rabbit

build/ros/rabbit: build/ros/Makefile
	@$(MAKE) -C build/ros

build/ros/Makefile: ros/CMakeLists.txt
	@mkdir -p build/ros
	@cd build/ros && cmake ../../ros

endif

#
# Set up sysroot for cross-compiling
#

.PHONY: sync-rpi4rootfs

sync-rpi4rootfs:
	@mkdir -p rpi4rootfs/usr
	@cd rpi4rootfs/usr && rsync -avh \
		$(RPI_HOST):/usr/include $(RPI_HOST):/usr/lib $(RPI_HOST):/usr/share .
	@cd rpi4rootfs/usr/lib/aarch64-linux-gnu && \
		rm -f libblas.so.3 && \
		ln -s blas/libblas.so.3 libblas.so.3
	@cd rpi4rootfs/usr/lib/aarch64-linux-gnu && \
		rm -f liblapack.so.3 && \
		ln -s lapack/liblapack.so.3 liblapack.so.3
	@cd rpi4rootfs/usr/lib/aarch64-linux-gnu && \
		rm -f libpthread.so && \
		ln -s libpthread-2.31.so libpthread.so
	@cd rpi4rootfs/usr/lib/aarch64-linux-gnu && \
		rm -f librt.so && \
		ln -s librt.so.1 librt.so
	@cd rpi4rootfs && rm -f lib && ln -s usr/lib lib

.PHONY: sync-rpi4rootfs

sync-rpi2rootfs:
	@mkdir -p rpi2rootfs/usr
	@cd rpi2rootfs/usr && rsync -avh \
		$(RPI_HOST):/usr/include $(RPI_HOST):/usr/lib $(RPI_HOST):/usr/share .
	@cd rpi2rootfs/usr/lib/arm-linux-gnueabihf && \
		rm -f libblas.so.3 && \
		ln -s blas/libblas.so.3 libblas.so.3
	@cd rpi2rootfs/usr/lib/arm-linux-gnueabihf && \
		rm -f liblapack.so.3 && \
		ln -s lapack/liblapack.so.3 liblapack.so.3
	@cd rpi2rootfs/usr/lib/arm-linux-gnueabihf && \
		rm -f libpthread.so && \
		ln -s libpthread-2.31.so libpthread.so
	@cd rpi2rootfs/usr/lib/arm-linux-gnueabihf && \
		rm -f librt.so && \
		ln -s librt.so.1 librt.so
	@cd rpi2rootfs && rm -f lib && ln -s usr/lib lib

#
# Build RPi Pico MCU
#

.PHONY: mcu

mcu: build/pico/rabbit_mcu.uf2

build/pico/rabbit_mcu.uf2: build/pico/Makefile
	$(MAKE) -C build/pico

build/pico/Makefile: mcu/CMakeLists.txt
	@mkdir -p build/pico
	@cd build/pico && PICO_SDK=$(PICO_SDK) PICO_SDK_PATH=$(PICO_SDK) \
		cmake ../../mcu

.PHONY: flash-mcu

flash-mcu: build/pico/rabbit_mcu.uf2
	@scp $< magpie:/H:/

#
# Build voicerec
#

.PHONY: voicerec

voicerec: build/voicerec/voicerec

build/voicerec/voicerec: build/voicerec/Makefile
	$(MAKE) -C build/voicerec

build/voicerec/Makefile: voicerec/CMakeLists.txt
	@mkdir -p build/voicerec
	@cd build/voicerec && cmake ../../voicerec

#
# Build voicerec2
#

.PHONY: voicerec2

voicerec: build/voicerec2/voicerec2

build/voicerec2/voicerec2: build/voicerec2/Makefile build/whisper.cpp/libwhisper.so 3rdparty/whisper.cpp/models/ggml-base.en.bin
	$(MAKE) -C build/voicerec2

build/voicerec2/Makefile: voicerec2/CMakeLists.txt
	@mkdir -p build/voicerec2
	@cd build/voicerec2 && cmake ../../voicerec2

#
# Builder whisper.cpp
#

.PHONY: whisper.cpp

whisper.cpp: build/whisper.cpp/libwhisper.so

build/whisper.cpp/libwhisper.so: build/whisper.cpp/Makefile
	$(MAKE) -C build/whisper.cpp

build/whisper.cpp/Makefile: 3rdparty/whisper.cpp/CMakeLists.txt
	@mkdir -p build/whisper.cpp
	@cd build/whisper.cpp && cmake ../../3rdparty/whisper.cpp

3rdparty/whisper.cpp/models/ggml-base.en.bin: 3rdparty/whisper.cpp/models/download-ggml-model.sh
	@bash $< base.en

#
# Build Intel's librealsense
#

.PHONY: librealsense

librealsense: build/$(ARCH)/librealsense/librealsense2.so

build/$(ARCH)/librealsense/librealsense2.so: build/$(ARCH)/librealsense/Makefile
	$(MAKE) -C build/$(ARCH)/librealsense

build/$(ARCH)/librealsense/Makefile: 3rdparty/librealsense/CMakeLists.txt
	@mkdir -p build/$(ARCH)/librealsense
	@cd build/$(ARCH)/librealsense && $(ARCH_ENVVARS) cmake \
		-DBUILD_GRAPHICAL_EXAMPLES=false \
		-DBUILD_EASYLOGGINGPP=false \
		-DBUILD_WITH_TM2=false \
		-DBUILD_GLSL_EXTENSIONS=false \
		-DFORCE_LIBUVC=true \
		-DCMAKE_BUILD_TYPE=release \
		$(CMAKE_EXTRAS) \
		../../../3rdparty/librealsense
