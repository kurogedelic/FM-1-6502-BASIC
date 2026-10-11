# FM-1 6502 BASIC

> **Status:** BASIC has been booted on a real FM-1 using an XIAO RP2040 FM-1 Transporter. The current firmware and input handling remain experimental; reliable cold boot, cursor visibility, and all panel-button scan mappings are not yet verified. Host tests: `make test basic-test`.

A small 6502 BASIC computer running directly on the M-VAVE FM-1.

The first goal is simple: boot Microsoft 6502 BASIC, type a program using the FM-1 keys, and run it on the built-in LCD. This is **not** an Apple II emulator. No Apple ROM or Apple II hardware is required.

## First milestone

On real FM-1 hardware:

```basic
10 PRINT "HELLO WORLD"
20 PRINT 2+2
RUN
```

The LCD shows `HELLO WORLD` and `4`. The program can be entered and edited entirely with the FM-1 keyboard and panel buttons.

## Components

- **Hardware support:** reuse `hugelton/felucca-bsp` for boot, LCD, key scanning, timers, and flash as appropriate.
- **Reference firmware:** `hugelton/Felucca` is GPL-3.0-licensed and serves as a technical reference for text rendering, input, packaging, and the MIDI SysEx updater. Any copied code must comply with its applicable license, including attribution and source-distribution requirements. This README does not grant separate permission to use contributors' or third-party code.
- **CPU:** use a small 6502 emulator. Evaluate the CPU core in `Keitark/fm1-nes`, including its license and 2A03-specific differences, before choosing it. Standard 6502 behavior is required.
- **BASIC:** Microsoft's open-source `microsoft/BASIC-M6502`, with its actual license and build configuration verified before bundling. Supply the target-specific initialization, memory layout, character input/output, and any required hooks.
- **Virtual machine:** a 64 KiB 6502 address space, with BASIC ROM, RAM, and simple memory-mapped terminal I/O. Exact addresses are TBD.

## Display

Use the FM-1's 240 × 240 LCD as a monochrome text terminal. Target a 40 × 24 character grid using a 6 × 8 font (240 × 192 pixels), with room for status information. Implement a cursor, scrolling, line editing, and printable ASCII. Reuse Felucca's LCD primitives where appropriate; no synthesizer UI is needed.

## Keyboard

The 27 piano keys are mapped **left to right** (F3 to G5). Use the following single-line layouts for labeling the keys; each character corresponds to one physical key.

**ABC layer** (default; 27th key is space):

```text
Q W E R T Y U I O P A S D F G H J K L Z X C V B N M SPACE
```

**Numbers / symbols** (press SAVE to toggle):

```text
1 2 3 4 5 6 7 8 9 0 " $ ( ) + - * / ^ = < > , . : ; ?
```

**SHIFT + symbols:** the first 15 keys produce `! @ # % & ' ~ | \\ _ \` [ ] { }` respectively; the remaining symbol keys are unchanged. SHIFT + letters produces lowercase letters. SHIFT and CTRL are toggles, not hold-to-use modifiers.

Panel assignments implemented in `src/keyboard.c`:

| FM-1 button | Action |
| --- | --- |
| FX | Tab |
| SCL | Toggle SHIFT |
| ENV | Enter |
| LFO | Backspace |
| EDIT | Space |
| GLO | Toggle CTRL |
| HOME | Escape |
| SAVE | Toggle ABC / symbols |
| ARP / SEQ | Cursor left / right |
| PLAY / REC | Cursor up / down |
| OCT− | BASIC BREAK (Ctrl-C) |
| OCT+ | Clear current input line (Ctrl-U) |

**Hardware caveat:** the panel label-to-scan-ID mapping is still provisional. Cursor-key events are private input events; actual editing behavior and the visible cursor need further hardware testing. Do not assume every panel control is verified.

## Build and installation

Build and package with the JieLi toolchain:

```sh
JIELI_TOOLCHAIN=~/.jieli/toolchain firmware/build.sh
JIELI_TOOLCHAIN=~/.jieli/toolchain firmware/package.sh
# Output: build/firmware/fm1basic.fwsc
```

### Normal installation (FM-1 detected over USB MIDI)

With a working FM-1 firmware/update interface and a USB data cable:

```sh
build/install-venv/bin/python third_party/felucca-bsp/tools/fm1_install.py build/firmware/fm1basic.fwsc
```

This requires the FM-1 MIDI interface to be detected. **It will not work when the device is only accessible through the Transporter in UBOOT**; an error such as `FM-1 not found` is expected in that state.

### Recovery / installation through the FM-1 Transporter (XIAO RP2040)

For a device stuck in UBOOT, use the XIAO RP2040 Transporter connected to the FM-1's USB D+, D− and GND. The recovery script lives under `build/MvaveFM1Unbricker/` after the recovery tools have been set up. See its `--help` and `setup` command to prepare the Transporter and embed the required JieLi loader.

**1. Identify the FM-1 and make a full backup before writing:**

```sh
PY=build/install-venv/bin/python
REC=build/MvaveFM1Unbricker/fm1_transporter_recover.py

$PY $REC info
$PY $REC dump build/basic-backup
```

`info` should identify chip key `980F`, flash ID `856014` (1 MiB). `dump` reads the full flash twice and compares SHA-256 checksums. Keep the resulting `fm1-transporter-dump-*.bin` file somewhere safe; it contains device-specific data. **Do not use a dry-run/mock dump as the reference for a real device.**

**2. Preview the write plan** using the exact dump filename printed by step 1:

```sh
REF=build/basic-backup/fm1-transporter-dump-YYYYMMDD-HHMMSS.bin
$PY $REC restore build/firmware/fm1basic.fwsc --ref "$REF"
```

Check that the fresh read matches the reference and that the planned changes are restricted to application sectors `0x04000..0x92FFF`. The SPL/head `0x00000..0x03FFF` must never be written; device-data sectors must remain untouched. The number of changed sectors depends on the firmware already installed.

**3. Write and verify** only if the preview is correct:

```sh
$PY $REC restore build/firmware/fm1basic.fwsc --ref "$REF" --write
```

Type `WRITE` at the confirmation prompt. Alternatively, `--write --yes` skips the prompt; use that only after checking the dry-run output. Do not disconnect power during writing. A cancelled confirmation means **nothing was written**.

The `--verify-v15` option is for restoring verified **official V15** firmware, **not** for installing this third-party BASIC package. Do not add it here.

A successful flash/verify does not by itself prove that the BASIC application boots reliably. Test the LCD, typing, cursor, and a small BASIC program after writing. Keep the Transporter and backup available for recovery.

## Implementation order

1. Make a standalone FM-1 firmware build from the BSP, using Felucca's proven build and packaging workflow as reference.
2. Initialize the LCD and key scanner; display a scrolling text terminal.
3. Implement QWERTY, number/symbol layers, and panel special keys. Verify all BASIC punctuation can be entered.
4. Bring up the 6502 emulator with RAM, ROM mapping, reset vectors, and a tiny diagnostic ROM.
5. Build Microsoft 6502 BASIC for the virtual machine; connect character input/output and memory initialization.
6. Reach the interactive BASIC prompt, enter `PRINT 2+2`, and execute a numbered BASIC program.
7. Package and install over MIDI SysEx; test repeated update and recovery on real hardware.
8. Later: optional save/load of BASIC programs in flash, USB serial terminal, and sound commands.

## Non-goals for v0.1

Apple II compatibility, graphics modes, disk emulation, synthesizer engines, MIDI sequencing, and a full-screen editor.

## Licensing

Felucca is licensed under GPL-3.0. Reusing its GPL-covered code in a distributed combined work requires compliance with the GPL, including providing the corresponding source and retaining required copyright and license notices. The author can separately license only code for which they hold the necessary rights; this does not extend to other contributors' or third-party code.\n\n`felucca-bsp` and the bundled build/update components may have different licenses, and some components (including `chip/usb.c`) are GPL-3.0-only. Check the licenses and provenance of each included component, including the BASIC ROM and CPU emulator, before distributing firmware binaries. **No blanket relicensing permission is granted by this README.**

## References

- https://github.com/hugelton/felucca-bsp
- https://github.com/hugelton/Felucca
- https://github.com/Keitark/fm1-nes
- https://github.com/microsoft/BASIC-M6502
