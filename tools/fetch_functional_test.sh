#!/bin/sh
# Download Klaus Dormann's 6502 functional test image (GPL-3.0; not bundled in this repo).
# Source: https://github.com/Klaus2m5/6502_65C02_functional_tests
set -eu
out=${1:-build/6502_functional_test.bin}
sha=fa12bfc761e6f9057e4cc01a665a7b800ff01ae91f598af1e39a1201d01953fd
mkdir -p "$(dirname "$out")"
curl -fsSL -o "$out" https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/master/bin_files/6502_functional_test.bin
got=$( (sha256sum "$out" 2>/dev/null || shasum -a 256 "$out") | cut -d' ' -f1)
[ "$got" = "$sha" ] || { echo "checksum mismatch: $got" >&2; rm -f "$out"; exit 1; }
