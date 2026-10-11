# Next integration steps

A small, **incomplete** 6502 execution core is now available for smoke testing. It handles enough instructions to execute a tiny ROM that writes text through $D000. It does **not** run Microsoft BASIC yet.

The interpreter deliberately traps unsupported opcodes, and currently omits decimal arithmetic, full addressing modes, IRQ/NMI, cycle-accurate timing, and genuine BRK behavior. It must not be represented as a complete 6502 emulator.

Microsoft's published BASIC-M6502 is historical MACRO-10 assembly, not a ready-to-flash binary. A practical next step is to adapt the ca65 conversion approach from rene6502/BASIC-M6502-CC65, build an appropriate target image, and connect its character I/O to this VM. That work requires a full 6502 instruction implementation.

After host execution works: integrate felucca-bsp LCD/key scan, reuse Felucca's licensed loader/package workflow, and test SysEx installation on hardware.

References:
- https://github.com/microsoft/BASIC-M6502
- https://github.com/rene6502/BASIC-M6502-CC65
