CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O2
COMMON = src/terminal.c src/keyboard.c src/vm.c
.PHONY: test clean smoke console-rom basic-rom basic-test
test: build/test_host build/test_cpu
	./build/test_host
	./build/test_cpu
build/test_host: tests/test_host.c $(COMMON)
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^
build/test_cpu: tests/test_cpu.c $(COMMON) src/cpu6502.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^
build/runner: src/runner.c $(COMMON) src/cpu6502.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^
build/smoke.bin: tools/make_smoke_rom.py
	python3 tools/make_smoke_rom.py $@
smoke: build/runner build/smoke.bin
	./build/runner build/smoke.bin 8000
build/console.o: rom/console.s
	mkdir -p build
	ca65 -o $@ $<
build/console.bin: build/console.o rom/console.cfg
	ld65 -C rom/console.cfg -o $@ build/console.o
console-rom: build/console.bin
build/m6502-fm1.s: third_party/BASIC-M6502-CC65/m6502-min.s rom/m6502-fm1.patch
	mkdir -p build
	patch -p1 -o $@ third_party/BASIC-M6502-CC65/m6502-min.s < rom/m6502-fm1.patch
build/basic.o: rom/basic.s build/m6502-fm1.s
	ca65 --feature force_range -I build -o $@ rom/basic.s
build/kernal.o: rom/kernal.s
	mkdir -p build
	ca65 -o $@ $<
build/basic.bin: build/basic.o build/kernal.o rom/basic.cfg
	ld65 -C rom/basic.cfg -m build/basic.map -o $@ build/basic.o build/kernal.o
basic-rom: build/basic.bin
basic-test: build/runner build/basic.bin
	python3 tests/test_basic.py
clean:
	rm -rf build
