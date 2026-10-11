CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O2
COMMON = src/terminal.c src/keyboard.c src/vm.c
.PHONY: test clean
test: build/test_host build/test_cpu
	./build/test_host
	./build/test_cpu
build/test_host: tests/test_host.c $(COMMON)
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^
build/test_cpu: tests/test_cpu.c $(COMMON) src/cpu6502.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -rf build
