#!/usr/bin/env python3
"""Send a plain-text BASIC listing to the FM-1 USB=1 CDC serial interface.

Requires: python3 -m pip install pyserial
Example: python3 tools/fm1basic_send.py /dev/cu.usbmodemXXXX hello.bas
"""
import argparse
import pathlib
import time

import serial


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("port", help="FM-1 BASIC CDC serial port (not the Transporter port)")
    p.add_argument("file", type=pathlib.Path, help="ASCII BASIC listing")
    p.add_argument("--delay", type=float, default=0.35, help="seconds between lines (default: 0.35)")
    p.add_argument("--keep", action="store_true", help="do not send NEW first")
    a = p.parse_args()
    if a.delay < 0:
        p.error("--delay must be nonnegative")
    lines = a.file.read_text(encoding="ascii").splitlines()
    for i, line in enumerate(lines, 1):
        if len(line) > 80:
            p.error(f"line {i} exceeds the 80-character BASIC input limit")
        if any(ord(c) < 32 or ord(c) > 126 for c in line):
            p.error(f"line {i} contains unsupported characters")
    with serial.Serial(a.port, 115200, timeout=0.1, write_timeout=3) as port:
        time.sleep(1)
        if not a.keep:
            port.write(b"NEW\r")
            port.flush()
            time.sleep(max(a.delay, 0.5))
        for line in lines:
            if not line.strip():
                continue
            port.write(line.encode("ascii") + b"\r")
            port.flush()
            time.sleep(a.delay)
        print(f"Sent {len(lines)} lines to {a.port}; type RUN on the FM-1.")
        print("Transfer is paced but not acknowledged; verify the listing with LIST before RUN.")


if __name__ == "__main__":
    main()
