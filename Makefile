# Makefile
#
# Copyright (C) 2023, Charles Chiou

ARCH :=		$(shell uname -m)

MAKEFLAGS =	--no-print-dir

TARGETS +=	build/$(ARCH)/rabbit

.PHONY: default clean distclean $(TARGETS)

default: $(TARGETS)

clean:
	@test -f build/$(ARCH)/Makefile && $(MAKE) -C build clean

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
	@cd build/x86_64 && cmake ../..

.PHONY: run run-ns attach install

run: build/aarch64/rabbit
	@if [ `hostname` = "rabbit" ]; then \
		sudo killall rabbit >/dev/null 2>&1; \
		sudo screen -R rabbit ./build/aarch64/rabbit; \
	fi

run-ns: build/aarch64/rabbit
	@if [ `hostname` = "rabbit" ]; then \
		sudo killall rabbit >/dev/null 2>&1; \
		sudo ./build/aarch64/rabbit; \
	fi

attach:
	@if [ `hostname` = "rabbit" ]; then \
		sudo screen -x rabbit; \
	fi

install: build/aarch64/rabbit
	@if [ `hostname` = "rabbit" ]; then \
		sudo killall rabbit >/dev/null 2>&1; \
		sudo install -m 755 $< /usr/local/bin; \
	fi
