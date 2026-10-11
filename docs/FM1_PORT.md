# FM-1 hardware port

**Status: builds, packages, simulated only. Nothing in this document has been run on a real FM-1.**
Do not describe any of it as working hardware until someone has tested it on a device.

## What exists

| Piece | Where | Verified how |
| --- | --- | --- |
| 6502 core, VM, Microsoft BASIC ROM | `src/`, `rom/`, `third_party/` | host tests, Dormann functional test, CI |
| Terminal model (40x24, CR LF handling) | `src/terminal.c` | `tests/test_ui.c`, `tests/test_basic.py --screen` checks |
| LCD text renderer (6x8 cells, 5x7 font) | `src/lcdtext.c` | pixel-exact snapshot test (`tests/test_ui.c`) |
| Keyboard map: 27 keys + panel buttons | `src/keyboard.c` | `tests/test_ui.c` (all BASIC characters reachable), `tests/test_keys.c` types the milestone program with FM-1 keys only |
| Firmware app (`firmware/fm1_basic.c`) | links to `build/firmware/fm1basic.bin` (42 KB, RAM 72 KB of 96 KB) | cross-compiles and links with JieLi's clang; **never executed** |
| Installable package | `firmware/package.sh` -> `build/firmware/fm1basic.fwsc` (identity `FM-1_990`) | parsed by felucca-bsp's installer code; felucca-bsp's simulated-device install test passes; **no device** |

## Build

```sh
tools/get_bsp.sh                       # felucca-bsp (MPL-2.0) at a pinned commit, into third_party/
tools/get_toolchain.sh                 # JieLi Linux toolchain (~26 MB) into ~/.jieli
make basic-rom                         # build/basic.bin
JIELI_TOOLCHAIN=~/.jieli/toolchain firmware/build.sh      # build/firmware/fm1basic.bin
JIELI_TOOLCHAIN=~/.jieli/toolchain firmware/package.sh    # build/firmware/fm1basic.fwsc
```

On macOS/arm64 the toolchain (Linux x86-64) runs in a `linux/amd64` Docker container (tested with colima + Rosetta); the
source tree must be under `$HOME`. `package.sh` additionally fetches Felucca (`tools/get_felucca.sh`, pinned) to run its
proven update-loader build, and three JieLi SDK files (`tools/get_sdk_files.sh`, SHA-256 checked, from JieLi's
gitee mirror; the GitHub URL in felucca-bsp's `get_sdk.sh` no longer resolves).

## Reuse and licensing

- felucca-bsp headers (clock, IRQ, timer, key scan/LEDs, LCD pins/SPI, boot guard, UBOOT entry) are **included at build time** from a
  pinned checkout; nothing is copied into this repository. They are MPL-2.0 (provisional per felucca-bsp's LICENSING.md).
- `firmware/fm1_basic.c` follows `examples/hwtest/hwtest.c` (same author) and is marked MPL-2.0.
- **`chip/usb.c` (GPL-3.0-only) is not used.** The firmware has no USB code, so the app image carries no GPL code from
  felucca-bsp. Consequence: no SysEx soft key (UBOOT request over USB) in the running app; see "Updating".
- Microsoft BASIC-M6502 is MIT (see `third_party/`).
- The **package** embeds Felucca's update loader (GPL-3.0-only) and three Apache-2.0 JieLi SDK files. The project's own
  license is still undecided; do not publish a `.fwsc` without shipping the matching licenses and loader source. CI therefore
  only uploads the app image, never the package.
- The 5x7 font in `src/lcdtext_font.inc` is a generic dot-matrix set written for this project.

## Hardware behavior (as designed, not as measured)

- LCD: ST7789 240x240 initialised with felucca-bsp's sequence (copied from hwtest). Text rows 0-23 = terminal (green on black),
  row 29 = status line (`ABC/123`, SHIFT, CTRL). Only changed rows are redrawn; the cursor blinks at ~1 Hz.
- Keys: TIMER5 at 10 kHz runs the matrix scan. Note keys index 0-26 -> QWERTY (letters) or digits/symbols (symbol layer).
  Buttons use felucca-bsp's matrix ids (0 = OCT-, 1 = OCT+, 2-13 = FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC).
  **The printed-label order for ids 2-11 is a best guess in Felucca too (only PLAY/REC were measured); check each button.**
  SCL = shift, SAVE = symbol layer, ENV = Enter, LFO = backspace, EDIT = space, GLO = Ctrl, OCT- = Ctrl-C (BREAK), OCT+ = Ctrl-U (erase line).
  Shift gives lowercase letters, but the platform layer folds typed input to uppercase.
- The 6502 runs in time slices of 4000 instructions between UI updates; speed on the device is **unmeasured**.
- If the emulator hits an unsupported opcode the firmware restarts BASIC.

## Updating and recovery

- First install and updates use the existing **UBOOT / MIDI SysEx** path: put the FM-1 in UBOOT, then
  `python3 third_party/felucca-bsp/tools/fm1_install.py build/firmware/fm1basic.fwsc` (needs `mido`, `python-rtmidi`).
  The installer, loader and package builder are felucca-bsp/Felucca's; **no new flashing protocol was written.**
- Entering UBOOT from this firmware: two consecutive boots that do not reach the 2 s healthy point enter UBOOT automatically
  (the hwtest guard); holding OCT- and OCT+ together for 2 s also does (`fm1_enter_uboot()`). Both are untested.
- This firmware does not touch the bootloader/recovery regions: the package is produced by felucca-bsp's `fm1pkg.py`, whose head
  area "is never written by the update loader". Return to official firmware as described by felucca-bsp/Felucca (M-VAVE's updater or
  FM-1-transporter).
- Before the first install: use a spare device if possible, and read felucca-bsp's `docs/EXPERIMENTAL.md`.

## Next hardware steps (need a device)

1. Flash `hwtest.bin` from felucca-bsp first to confirm the toolchain/loader/installer chain on your unit.
2. Flash `fm1basic.fwsc`; expected: banner and `READY.` on the LCD. If not, reset twice to reach UBOOT.
3. Confirm each panel button's label/ID, key feel (no auto-repeat yet), and 6502 speed; then fix the tables.
4. Optional later: flash-backed `LOAD`/`SAVE`, a USB serial terminal (needs a non-GPL USB or a GPL decision).
