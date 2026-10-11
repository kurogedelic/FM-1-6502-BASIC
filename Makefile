CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O2
.PHONY: test clean
test: build/test_host
	./build/test_host
build/test_host: tests/test_host.c src/terminal.c src/keyboard.c src/vm.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -rf build
