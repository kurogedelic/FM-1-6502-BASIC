# ROM build

Tools: `ca65`/`ld65` (cc65 2.18+), `patch`, `make`, a C99 compiler, Python 3.

```sh
make basic-rom     # build/basic.bin (32 KiB image for $8000-$FFFF), build/basic.map
make basic-test    # runs tests/test_basic.py against build/runner
make console-rom   # the older diagnostic echo console (rom/console.s)
make test smoke    # host unit tests and tiny smoke ROM
```

Run it by hand (type at the terminal; the runner streams stdin to `$D001/$D000`):

```sh
printf '10 PRINT "HELLO FM-1"\rRUN\r' | ./build/runner --exit-on-idle build/basic.bin
```

Runner options: `--exit-on-idle` (exit 0 when stdin is at EOF and the ROM is
waiting for a key), `--max-steps N`. A ROM covering the vector area supplies its own vectors.

## BASIC memory map (`rom/basic.cfg`)

| Range | Use |
| --- | --- |
| `$0000-$008B` | BASIC zero page |
| `$0100-$01FF` | stack / number formatting buffer |
| `$0200-$02FF` | BASIC input buffer |
| `$0300-$03FF` | platform variables (type-ahead ring, line editor) |
| `$0400-$7FFF` | program and variable RAM |
| `$8000-$A024` | Microsoft BASIC |
| `$A025-$A16A` | platform layer (`rom/kernal.s`) |
| `$D000` / `$D001` | terminal data / status registers (override ROM) |
| `$FFC0-$FFE9` | platform jump table |
| `$FFFA-$FFFF` | NMI / RESET / IRQ vectors |

## Terminal I/O contract

- read `$D001`: 1 when a character is waiting, else 0;
- read `$D000`: dequeue one input character;
- write `$D000`: emit one output character (CR LF ends a line, BS erases).

## Console ROM

`make console-rom` still builds `rom/console.s`, the tiny echo ROM used as an
I/O diagnostic before BASIC existed. It is not BASIC.
