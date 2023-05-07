# Makefile
#
# Copyright (C) 2023, Charles Chiou

ARCH :=		$(shell uname -m)
MAKEFLAGS =	--no-print-dir
RPI_HOST ?=	rabbit

TARGETS +=	build/$(ARCH)/rabbit

.PHONY: default clean distclean $(TARGETS)

default: $(TARGETS)

clean:
	@test -f build/$(ARCH)/Makefile && $(MAKE) -C build/$(ARCH) clean

distclean:
	rm -rf build

build/aarch64/rabbit: build/aarch64/Makefile
	@$(MAKE) -C build/aarch64

build/aarch64/Makefile: CMakeLists.txt
	@mkdir -p build/aarch64
	@cd build/aarch64 && cmake ../..

build/x86_64/rabbit: build/x86_64/Makefile
	@$(MAKE) -C build/x86_64

build/x86_64/Makefile: CMakeLists.txt
	@mkdir -p build/x86_64
	@cd build/x86_64 && CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++ cmake ../..

.PHONY: run install

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
	fi

#
# Set up sysroot for cross-compiling
#

.PHONY: sync-rpi4rootfs

sync-rpi4rootfs:
	@mkdir -p rpi4rootfs/usr
	@cd rpi4rootfs/usr && rsync -avh \
		$(RPI_HOST):/usr/include $(RPI_HOST):/usr/lib .
	@cd rpi4rootfs/usr/lib/aarch64-linux-gnu && \
		rm -f libblas.so.3 && \
		ln -s blas/libblas.so.3 libblas.so.3
	@cd rpi4rootfs/usr/lib/aarch64-linux-gnu && \
		rm -f liblapack.so.3 && \
		ln -s lapack/liblapack.so.3 liblapack.so.3
	@cd rpi4rootfs/usr/lib/aarch64-linux-gnu && \
		rm -f libpthread.so && \
		ln -s libpthread-2.31.so libpthread.so
	@cd rpi4rootfs && rm -d lib && ln -s usr/lib lib
