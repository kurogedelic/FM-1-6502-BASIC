# Implementation status

This is an initial, host-testable implementation, **not yet bootable firmware**.

Implemented:
- 40×24 scrolling ASCII text terminal and cursor
- 27-key QWERTY letter layer and number/punctuation layer
- 12 panel-button actions (provisional mapping)
- 64 KiB virtual memory, ROM loading, and byte-oriented terminal I/O
- host unit tests (`make test`)

Not implemented yet:
- complete 6502 CPU execution (choose or port an appropriately licensed core)
- assembling/adapting Microsoft's BASIC-M6502 source and I/O vectors
- FM-1 LCD renderer, key-scan adapter, boot entry and linker script
- SysEx/uboot update package integration and real-device verification

The virtual I/O registers at 0xD000/0xD001 are project-specific, **not Apple II I/O**. A BASIC build will need input/output stubs targeting these registers, or direct host callbacks.

Safety: do not flash this project as firmware. The host tests only check software behavior; no device image is produced.
