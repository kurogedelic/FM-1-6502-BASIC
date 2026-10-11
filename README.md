# FM-1 6502 BASIC

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
- **Reference firmware:** reuse relevant code from `hugelton/Felucca` for text rendering, input handling, firmware packaging, and the MIDI SysEx updater. The project author permits reuse of their own code; retain notices and verify rights for contributions and third-party components.
- **CPU:** use a small 6502 emulator. Evaluate the CPU core in `Keitark/fm1-nes`, including its license and 2A03-specific differences, before choosing it. Standard 6502 behavior is required.
- **BASIC:** Microsoft's open-source `microsoft/BASIC-M6502`, with its actual license and build configuration verified before bundling. Supply the target-specific initialization, memory layout, character input/output, and any required hooks.
- **Virtual machine:** a 64 KiB 6502 address space, with BASIC ROM, RAM, and simple memory-mapped terminal I/O. Exact addresses are TBD.

## Display

Use the FM-1's 240 × 240 LCD as a monochrome text terminal. Target a 40 × 24 character grid using a 6 × 8 font (240 × 192 pixels), with room for status information. Implement a cursor, scrolling, line editing, and printable ASCII. Reuse Felucca's LCD primitives where appropriate; no synthesizer UI is needed.

## Keyboard

Map the FM-1's 27 piano keys to a compact **QWERTY** text keyboard. The proposed default layer is:

```text
Q W E R T Y U I O P
 A S D F G H J K L
  Z X C V B N M
```

This uses 26 keys; the 27th key is reserved for a frequently used character or modifier, to be decided after checking the physical key order.

Use panel buttons for special keys. **Provisional** mapping (verify actual scan IDs and ergonomics against Felucca):

| Panel button | Terminal action |
| --- | --- |
| FX | Tab |
| SCL | Shift (hold or latch) |
| ENV | Enter |
| LFO | Backspace / Delete |
| EDIT | Space |
| GLO | Ctrl (hold or latch) |
| HOME | Escape |
| SAVE | Symbol / number layer |
| ARP | Cursor left |
| SEQ | Cursor right |
| PLAY | Cursor up |
| REC | Cursor down |

The OCT− and OCT+ buttons can change keyboard layers (letters / numbers and punctuation), or move within a character selection. Final behavior should be tested on hardware. BASIC needs digits, quotation marks, colon, arithmetic operators, parentheses, and `?` as well as letters; these must be accessible without a computer.

Support uppercase ASCII first. Show the active layer and modifier state on screen. Do not assume the FM-1 has a physical QWERTY keyboard.

## Build and installation

Use the JieLi toolchain and FM-1 firmware packaging established by Felucca. Build an installable firmware package and support **automatic flashing via MIDI SysEx through the existing Felucca update-loader / uboot workflow**, using the proven package and installer code rather than inventing a new flashing protocol.

Before implementation, check the loader handshake, package identity, size limits, and what must be preserved for recovery. Do not overwrite bootloader or recovery areas blindly. Validate on a test device and document recovery with FM-1-transporter.

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

This repository is a new firmware project. The author of Felucca may grant permission for code they own, but cannot unilaterally relicense other contributors' code. `felucca-bsp` currently marks parts of its licensing as provisional and `chip/usb.c` as GPL-3.0-only. Review the exact files imported, their notices, and the license of the BASIC and CPU emulator before choosing the project's final distribution license.

## References

- https://github.com/hugelton/felucca-bsp
- https://github.com/hugelton/Felucca
- https://github.com/Keitark/fm1-nes
- https://github.com/microsoft/BASIC-M6502
