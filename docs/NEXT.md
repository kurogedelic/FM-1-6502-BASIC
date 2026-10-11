# Next steps

1. FM-1 hardware bring-up (see `docs/FM1_PORT.md`): felucca-bsp build, LCD text
   renderer driven by `src/terminal.c`, key scanner feeding `src/keyboard.c`,
   6502 core executing `build/basic.bin` on the device.
2. SysEx firmware packaging using the existing Felucca update loader; test on a
   spare device with a recovery path before anything else.
3. Optional: flash-backed `LOAD`/`SAVE`, interrupt-driven input, sound statements.
