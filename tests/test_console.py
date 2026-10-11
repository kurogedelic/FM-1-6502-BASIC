#!/usr/bin/env python3
"""End-to-end ca65 ROM / CPU / memory-mapped I/O integration test."""
import subprocess
from pathlib import Path

rom = Path("build/console.bin").read_bytes()
assert len(rom) == 32768, len(rom)
assert rom[-4:-2] == b"\x00\x80", "reset vector must point to $8000"
result = subprocess.run(
    ["build/runner", "build/console.bin", "8000"],
    input=b"HELLO\n",
    capture_output=True,
    timeout=15,
    check=False,
)
assert result.returncode == 1, (result.returncode, result.stderr)
assert b"FM-1 6502 CONSOLE\r\n" in result.stdout, result.stdout
assert b"HELLO\r\n" in result.stdout, result.stdout
assert b"Execution budget exhausted" in result.stderr, result.stderr
print("6502 console ROM integration: PASS")
