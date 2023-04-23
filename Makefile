# Makefile
#
# Copyright (C) 2023, Charles Chiou

MAKEFLAGS =	--no-print-dir

TARGETS +=	build/rabbit

.PHONY: default clean distclean $(TARGETS)

default: $(TARGETS)

clean:
	@test -f build/Makefile && $(MAKE) -C build clean

distclean:
	rm -rf build

build/rabbit: build/Makefile
	@$(MAKE) -C build

build/Makefile: CMakeLists.txt
	@mkdir -p build
	@cd build && cmake ..

.PHONY: run run-ns attach install

run: build/rabbit
	@if [ `hostname` = "rabbit" ]; then \
		sudo killall rabbit >/dev/null 2>&1; \
		sudo screen -R rabbit ./build/rabbit; \
	fi

run-ns: build/rabbit
	@if [ `hostname` = "rabbit" ]; then \
		sudo killall rabbit >/dev/null 2>&1; \
		sudo ./build/rabbit; \
	fi

attach:
	@if [ `hostname` = "rabbit" ]; then \
		sudo screen -x rabbit; \
	fi

install: build/rabbit
	@if [ `hostname` = "rabbit" ]; then \
		sudo killall rabbit >/dev/null 2>&1; \
		sudo install -m 755 $< /usr/local/bin; \
	fi
