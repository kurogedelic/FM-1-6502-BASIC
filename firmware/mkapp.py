#!/usr/bin/env python3
"""mkapp.py TEXT.bin DATA.bin SYMS.txt OUT.bin
Assemble the app image the FM-1 SPL expects (same layout as felucca-bsp's hwtest build):
.text at XIP 0x02000120 followed by the initial .data image at _data_load."""
import re
import sys

text, data, syms, out = sys.argv[1:]
syms = open(syms).read()
load = int(re.search(r"^([0-9a-f]+) .*\s_data_load$", syms, re.M).group(1), 16)
start = int(re.search(r"^([0-9a-f]+) .*\s_start$", syms, re.M).group(1), 16)
assert start == 0x02000120, hex(start)
img = bytearray(open(text, "rb").read())
d = open(data, "rb").read()
if d:
    img += b"\xff" * (load - 0x02000120 - len(img)) + d
img += b"\xff" * (-len(img) % 4)
assert len(img) <= 0x8DFBC, f"app image {len(img)} B does not fit the app slot"
open(out, "wb").write(img)
print(f"{out}: {len(img)} bytes")
