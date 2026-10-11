#!/usr/bin/env python3
"""bin2c.py IN.bin OUT.c SYMBOL : embed a binary as `const unsigned char SYMBOL[]`."""
import sys
data = open(sys.argv[1], "rb").read()
with open(sys.argv[2], "w") as f:
    f.write(f"/* generated from {sys.argv[1]} by tools/bin2c.py */\n")
    f.write(f"const unsigned char {sys.argv[3]}[{len(data)}] = {{\n")
    for i in range(0, len(data), 16):
        f.write(",".join(str(b) for b in data[i:i + 16]) + ",\n")
    f.write("};\n")
