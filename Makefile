# Makefile
#
# Copyright (C) 2023, Charles Chiou

ARCH :=		$(shell uname -m)
MAKEFLAGS =	--no-print-dir
RPI_HOST ?=	rabbit

TARGETS =	build/$(ARCH)/rabbit

ifeq ($(ARCH),x86_64)
ARCH_ENVVARS =	CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++
endif

ifeq ($(ARCH),aarch64)
TARGETS +=	build/ros/rabbit_node
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

build/$(ARCH)/rabbit: build/$(ARCH)/Makefile
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

ifneq ($(findstring build/ros/rabbit_node,$(TARGETS)),)

.PHONY: ros

ros: build/ros/rabbit_node

build/ros/rabbit_node: build/ros/Makefile
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
	@cd rpi4rootfs && rm -d lib && ln -s usr/lib lib
