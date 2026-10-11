# Current state

Everything below is verified by `make test smoke basic-test cpu-functional`
(and the same targets in GitHub Actions). **Nothing here has been run on FM-1
hardware yet.**

## Working (host emulator)

- **6502 core** (`src/cpu6502.c`): all documented NMOS 6502 opcodes, real BRK,
  IRQ/NMI entry points, NMOS decimal-mode flag behavior. Passes Klaus
  Dormann's 6502 functional test (`make cpu-functional`, image downloaded and
  checksum-pinned, not bundled). Not cycle accurate (`cycles` counts
  instructions); undocumented opcodes stop the CPU.
- **Virtual machine**: 64 KiB address space, terminal registers `$D000/$D001`.
- **Microsoft BASIC-M6502 1.1** (cc65 port, `third_party/`) boots from the reset
  vector, prints its banner and `31743 BYTES FREE`, and runs:
  `PRINT 2+2` -> ` 4`; `10 PRINT "HELLO FM-1"` + `RUN`; floating point, strings,
  `FOR`/`NEXT`, `INPUT`, `GET`, `LIST`, Ctrl-C `BREAK`, line editing.
- **Platform layer** (`rom/kernal.s`): terminal I/O, line editor, type-ahead, Ctrl-C.
- **Host runner** (`build/runner`): streams stdin into the input queue; used by tests.
- **Terminal model / keyboard map** (`src/terminal.c`, `src/keyboard.c`): 40x24
  text grid, QWERTY + symbol layers, panel buttons (provisional mapping). Unit tested only.
- **CI**: host tests (gcc/clang), sanitizers, cppcheck, console ROM, BASIC ROM
  integration tests, CPU functional test.

## Not implemented / not verified

- FM-1 firmware: LCD renderer, key-scan adapter, JieLi build, boot entry.
  Requires `hugelton/felucca-bsp` and the JieLi toolchain; see `docs/FM1_PORT.md`.
- MIDI SysEx firmware package/installation (Felucca update-loader workflow).
- `LOAD`/`SAVE` (raise `?ILLEGAL QUANTITY ERROR`), sound, graphics.
- Cycle-accurate timing, undocumented opcodes.
- The shipped host runner uses a cooked tty if run interactively (the tty echoes too).
