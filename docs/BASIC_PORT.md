# Microsoft BASIC integration

Status: **working on the host emulator.** `make basic-rom` builds a 32 KiB ROM
that boots Microsoft BASIC-M6502 on the project's 6502 core; `make basic-test`
drives it through the `$D000/$D001` terminal registers.

## What is bundled

| Component | Source | License |
| --- | --- | --- |
| BASIC interpreter | `third_party/BASIC-M6502-CC65/m6502-min.s` (unmodified), from https://github.com/rene6502/BASIC-M6502-CC65 | MIT, (c) Microsoft Corporation |
| Original MACRO-10 source (reference) | `third_party/BASIC-M6502/m6502.asm` from https://github.com/microsoft/BASIC-M6502 | MIT, (c) Microsoft Corporation |

Pinned commits and the license texts are in `third_party/`. The interpreter is
Microsoft's code; nothing was replaced by a custom interpreter.

`m6502-min.s` is the cc65 port's variant with minimal hardware dependencies
(no external I/O, no TI$, no VIA-timer RND). It assembles to the same bytes as
the published `m6502-min.bin` (MD5 `b2b05e5b...`), which was verified before
any patching.

## FM-1 changes

1. `rom/m6502-fm1.patch` (applied by `make` into `build/m6502-fm1.s`): the only
   change is the sign-on banner (`### COMMODORE BASIC ###` -> `FM-1 MICROSOFT BASIC V1.1`).
2. `rom/basic.s`: wrapper that exports `STOP`, `INIT`, `FCERR` and includes the patched source.
3. `rom/kernal.s`: platform layer. The cc65 port expects Commodore KERNAL
   entry points; this file provides them at the same addresses ($FFC0-$FFE9)
   on top of the FM-1 terminal registers.
4. `rom/basic.cfg`: memory map (see `docs/ROM_BUILD.md`).

## Platform layer behavior (`rom/kernal.s`)

- **Reset**: vector at `$FFFC` -> kernal init -> BASIC `INIT` (cold start). BASIC
  probes RAM from `$0400` up to `$7FFF` (its limit), giving 31743 bytes free.
- **Output (`OUTCH`)**: CR is a newline and is sent as CR LF; LF from BASIC
  is dropped. `BS` (8) is a backspace-and-erase for the terminal.
- **Input (`CQINCH`)**: line-oriented with echo; BS/DEL erase one character,
  Ctrl-U erases the line, Ctrl-C abandons it, lowercase is folded to uppercase,
  80 character limit. Enter is not echoed (BASIC prints CR LF itself).
- **Ctrl-C while running (`ISCNTC`)**: drains pending input into a 16-byte
  type-ahead ring, enters BASIC's `STOP` on Ctrl-C (prints `BREAK`).
- **`GET` (`CZGETL`)**: non-blocking.
- **`LOAD`/`SAVE`/`VERIFY`**: not available, raise `?ILLEGAL QUANTITY ERROR`.
  (Flash storage is a later milestone.)

## Known limits

- This is Microsoft BASIC 1.1 (the 8K-era interpreter): no graphics or sound
  commands, and BASIC's RAM probe stops at `$7FFF` (32 KiB ceiling).
- `CHR$(10)` is swallowed by the kernal; BASIC's `TRMPOS` does not track the
  terminal's wrap at column 40.
- The CPU core is not cycle accurate and has no IRQ/NMI yet (see `docs/STATUS.md`).
