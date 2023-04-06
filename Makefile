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
