#!/usr/bin/env python3
"""Produce a small 6502 ROM for the host runner, no assembler needed."""
from pathlib import Path
import sys
target = Path(sys.argv[1]) if len(sys.argv)>1 else Path("build/smoke.bin")
target.parent.mkdir(parents=True, exist_ok=True)
rom = bytearray()
for char in b"HELLO 6502\r":
    rom.extend((0xA9, char, 0x8D, 0x00, 0xD0))
rom.append(0x00)
target.write_bytes(rom)
print(f"Wrote {target} ({len(rom)} bytes), load at $8000")
