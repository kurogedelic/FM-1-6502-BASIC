# Microsoft BASIC integration

The project now has a host-side ROM runner: `make smoke` produces and executes a tiny 6502 binary which prints through the virtual $D000 output register. This is **not** Microsoft BASIC.

Upstream source: https://github.com/microsoft/BASIC-M6502 (MIT). It is MACRO-10 style assembly with target-specific code. A ca65-oriented conversion exists at https://github.com/rene6502/BASIC-M6502-CC65; its `m6502-min.s` variant removes some Commodore hardware dependencies, but still needs adaptation to this machine.

Suggested port:
1. Reproduce the converter's ca65 build with source/license attribution.
2. Select a memory map that avoids virtual I/O at $D000/$D001, and reserve BASIC's zero-page and stack usage.
3. Patch BASIC's terminal input and output entry points to poll/read $D001/$D000 and write $D000.
4. Set the reset vector to the BASIC initialization entry and feed scripted ASCII input from the host.
5. Require `PRINT 2+2`, then numbered program input and `RUN` as integration tests.
6. Once that works, connect Felucca LCD and key scanning, then the update package.

Caveats: The CPU currently traps BRK and does not implement interrupts or cycle timing. BCD flags are approximate. These must be corrected before claiming complete 6502 compatibility.
