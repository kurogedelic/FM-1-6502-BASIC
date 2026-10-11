#!/usr/bin/env python3
"""Boot Microsoft BASIC-M6502 in the host 6502 emulator and drive it through
the memory-mapped terminal ($D000/$D001), exactly as the FM-1 firmware does."""
import subprocess
import sys

ROM = "build/basic.bin"
RUNNER = "build/runner"


def run(text, max_steps=30_000_000):
    """Feed `text` (a str; \\n becomes CR like the Enter key) and return the
    terminal output with CR LF normalised to LF."""
    data = text.replace("\n", "\r").encode("latin-1")
    result = subprocess.run(
        [RUNNER, "--exit-on-idle", "--max-steps", str(max_steps), ROM],
        input=data, capture_output=True, timeout=120, check=False)
    assert result.returncode == 0, (result.returncode, result.stderr, result.stdout)
    return result.stdout.decode("latin-1").replace("\r\n", "\n")


def screen(text, max_steps=30_000_000):
    """Final contents of the 40x24 terminal model (what the FM-1 LCD shows)."""
    data = text.replace("\n", "\r").encode("latin-1")
    result = subprocess.run(
        [RUNNER, "--exit-on-idle", "--screen", "--max-steps", str(max_steps), ROM],
        input=data, capture_output=True, timeout=120, check=False)
    assert result.returncode == 0, (result.returncode, result.stderr)
    return result.stdout.decode("latin-1").split("\n")[:-1]


def check(name, text, *expected, absent=()):
    out = run(text)
    for want in expected:
        assert want in out, f"{name}: missing {want!r} in\n{out}"
    for bad in absent:
        assert bad not in out, f"{name}: unexpected {bad!r} in\n{out}"
    print(f"PASS {name}")
    return out


# Size/layout sanity of the ROM image.
rom = open(ROM, "rb").read()
assert len(rom) == 32768, len(rom)
assert rom[0xFFFC - 0x8000:0xFFFE - 0x8000] != b"\xff\xff", "reset vector missing"
assert b"FM-1 MICROSOFT BASIC V1.1" in rom, "BASIC banner missing from ROM"

out = check("boot banner", "",
            "FM-1 MICROSOFT BASIC V1.1\n", " 31743 BYTES FREE\n", "\nREADY.\n")

check("PRINT 2+2", "PRINT 2+2\n", "\n 4 \n")

check("program: 10 PRINT HELLO FM-1 / RUN",
      '10 PRINT "HELLO FM-1"\nRUN\n', "\nHELLO FM-1\n")

check("two-line program with LIST",
      '10 PRINT "HELLO FM-1"\n20 PRINT 2+2\nRUN\nLIST\n',
      "HELLO FM-1\n 4 \n", '10 PRINT "HELLO FM-1"\n 20 PRINT 2+2\n')

check("lowercase input folds to uppercase", "print 6*7\n", "\n 42 \n")

check("floating point", "?SQR(2)*10\n", " 14.1421356 ")

check("strings", 'A$="ABC":?LEFT$(A$,2)+MID$(A$,2,1);LEN(A$)\n', "ABB 3 ")

check("FOR loop", "10 FOR I=1 TO 5:?I*I;:NEXT\nRUN\n", " 1  4  9  16  25 ")

check("long-running loop", "10 FOR I=1 TO 2000:S=S+I:NEXT\n20 PRINT S\nRUN\n", " 2001000 ")

check("INPUT", "10 INPUT X:?X*2\nRUN\n21\n", "? 21\n", " 42 ")

check("GET", '10 GET K$:IF K$="" THEN 10\n20 ?"KEY=";K$\nRUN\nQ', "KEY=Q")

check("backspace editing", "PRINT 1+9\x08\x082\n", "\n 12 \n", "\x08\x08")

check("Ctrl-U kills the line", 'PRINT 111\x15PRINT 222\n', "\n 222 \n", absent=(" 111 ",))

check("syntax error", "PRINT )\n", "?SYNTAX ERROR")

check("Ctrl-C breaks a running program",
      '10 GOTO 10\nRUN\n\x03PRINT "AFTER"\n', "BREAK\n", "\nAFTER\n")

check("type-ahead while a program runs is not lost",
      '10 FOR I=1 TO 50:NEXT\n20 PRINT "DONE"\nRUN\nPRINT 5+5\n', "DONE\n", " 10 ")

check("ROM is write protected", "A=PEEK(32768):POKE 32768,A+1:?A=PEEK(32768)\n", "\n-1 \n")
check("RAM is writable", "POKE 2048,7:?PEEK(2048)\n", "\n 7 \n")

check("LOAD is reported, not silently ignored", "LOAD\n", "?ILLEGAL QUANTITY ERROR")

# What the LCD terminal shows: CR LF must be one newline, and the 40-column grid must hold the screen.
lines = screen('10 PRINT "HELLO FM-1"\n20 PRINT 2+2\nRUN\n')
assert lines == [
    "FM-1 MICROSOFT BASIC V1.1",
    "",
    " 31743 BYTES FREE",
    "",
    "READY.",
    '10 PRINT "HELLO FM-1"',
    "20 PRINT 2+2",
    "RUN",
    "HELLO FM-1",
    " 4",
    "",
    "READY.",
], lines
print("PASS terminal screen after RUN")

lines = screen("10 FOR I=1 TO 30:PRINT I:NEXT\nRUN\n")
assert lines[-1] == "READY." and lines[-3] == " 30", lines[-4:]
assert len(lines) == 23, len(lines)   # 24th row holds the cursor
print("PASS terminal scrolls")

lines = screen("PRINT \"" + "X" * 40 + "\"\nPRINT 1\n")
assert "X" * 40 in lines and lines[lines.index("X" * 40) + 1] == "", lines
print("PASS 40-column line wraps without a stray blank line")

print("BASIC integration: ALL PASS")
