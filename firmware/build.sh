#!/bin/sh
# Build the FM-1 firmware image build/firmware/fm1basic.bin.
#   JIELI_TOOLCHAIN=~/.jieli/toolchain firmware/build.sh
# Needs: tools/get_bsp.sh (felucca-bsp), the JieLi toolchain (felucca's tools/get_toolchain.sh),
# `make basic-rom`, python3. The toolchain is Linux x86-64: on other hosts each tool runs in a
# linux/amd64 container (docker, files must be under $HOME).
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TC="${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN must point at the JieLi Linux toolchain}"
BSP="${BSP:-$ROOT/third_party/felucca-bsp}"
OUT="$ROOT/build/firmware"
[ -f "$BSP/chip/crt0.S" ] || { echo "felucca-bsp not found: run tools/get_bsp.sh" >&2; exit 1; }
[ -f "$ROOT/build/basic.bin" ] || make -C "$ROOT" basic-rom
rm -rf "$OUT"; mkdir -p "$OUT"
python3 "$ROOT/tools/bin2c.py" "$ROOT/build/basic.bin" "$OUT/rom_image.c" basic_rom

if [ "$(uname -s)-$(uname -m)" = "Linux-x86_64" ]; then
    run() { "$@"; }
else
    run() { docker run --rm --platform linux/amd64 -v "$HOME:$HOME" -w "$ROOT" \
        "${JIELI_DOCKER_IMAGE:-debian:bookworm-slim}" "$@"; }
fi
CC="$TC/pi32v2/bin/clang -target pi32v2"
CFLAGS="-Os -ffunction-sections -fno-builtin -std=gnu99 -Wall -Wno-unused-function -I$BSP/chip -I$BSP/boards/fm1 -I$ROOT/firmware/include -I$ROOT/src"
cc()  { run sh -c "$CC $CFLAGS $* "; }
cc_s() { run sh -c "$CC -c $1 -o $2"; }

cc_s "$BSP/chip/crt0.S"    "$OUT/crt0.o"
cc_s "$BSP/chip/fm1_vec.S" "$OUT/vec.o"
cc_s "$BSP/chip/fm1_isr.S" "$OUT/isr.o"
cc -c "$ROOT/firmware/fm1_basic.c" -o "$OUT/main.o"
for f in cpu6502 vm terminal keyboard lcdtext; do
    cc -c "$ROOT/src/$f.c" -o "$OUT/$f.o"
done
cc -c "$OUT/rom_image.c" -o "$OUT/rom_image.o"
run "$TC/pi32v2/bin/ld" -T "$BSP/examples/hwtest/app.ld" "$OUT/crt0.o" "$OUT/vec.o" "$OUT/isr.o" \
    "$OUT/main.o" "$OUT/cpu6502.o" "$OUT/vm.o" "$OUT/terminal.o" "$OUT/keyboard.o" "$OUT/lcdtext.o" \
    "$OUT/rom_image.o" -Map "$OUT/fm1basic.map" -o "$OUT/fm1basic.elf"
run "$TC/common/bin/objcopy" -O binary -j .text "$OUT/fm1basic.elf" "$OUT/text.bin"
run "$TC/common/bin/objcopy" -O binary -j .data "$OUT/fm1basic.elf" "$OUT/data.bin"
run "$TC/common/bin/objdump" -t "$OUT/fm1basic.elf" > "$OUT/syms.txt"
run "$TC/common/bin/objdump" -h "$OUT/fm1basic.elf" > "$OUT/sections.txt"
python3 "$ROOT/firmware/mkapp.py" "$OUT/text.bin" "$OUT/data.bin" "$OUT/syms.txt" "$OUT/fm1basic.bin"
cat "$OUT/sections.txt"
