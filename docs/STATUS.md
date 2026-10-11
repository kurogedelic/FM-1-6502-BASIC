# Current state

## Working components (subject to CI verification)

- C host 6502 emulator with many documented NMOS 6502 instructions; BRK is still a stop trap and decimal flags are approximate.
- 64 KiB virtual memory and character I/O at $D000/$D001.
- ca65/ld65-built console ROM with reset vector.
- Host test for console banner, piped keyboard input, echo, and newline conversion.

## Not implemented

- Microsoft BASIC source port and actual BASIC prompt.
- Interactive nonblocking host terminal (host runner currently preloads stdin until EOF).
- FM-1 LCD/keyboard driver integration, JieLi build, MIDI SysEx firmware packaging.
- Complete NMOS 6502 interrupt/cycle behavior.

The end-to-end console test expects the runner to hit its instruction budget because the ROM intentionally loops forever.
