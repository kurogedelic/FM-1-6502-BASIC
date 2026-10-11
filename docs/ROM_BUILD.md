# 6502 console ROM

`make console-rom` assembles `rom/console.s` with ca65 and links a 32 KiB image at $8000. The 6502 reset/interrupt vectors live at $FFFA-$FFFF.

The console demonstrates the I/O contract used by a future Microsoft BASIC port:

- read $D001: 1 when a queued character is available, otherwise 0;
- read $D000: dequeue one input character;
- write $D000: emit one output character.

The host runner can preload a short input stream from standard input:

```sh
printf 'HELLO\\n' | ./build/runner build/console.bin 8000
```

The console loops indefinitely, so the runner eventually exits with its instruction-budget diagnostic. This is expected for this initial interactive ROM. The console is not a BASIC interpreter.
