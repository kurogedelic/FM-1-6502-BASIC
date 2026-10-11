#!/bin/sh
# Build an installable FM-1 package (.fwsc). DIAG=1 and USB=1 select matching variants.
#   JIELI_TOOLCHAIN=~/.jieli/toolchain firmware/package.sh
# Reuses the PROVEN Felucca pipeline instead of a new flashing protocol:
#   * the update loader (ota.bin) is built by Felucca's own tools/build.py build_loader()
#   * the package is written by felucca-bsp's tools/fm1pkg.py (the one used for hwtest)
#   * the three JieLi SDK files it embeds are fetched and hash-checked by tools/get_sdk_files.sh
# The .fwsc contains Felucca's loader (GPL-3.0-only) and Apache-2.0 SDK files: keep it local unless you
# can ship the matching licenses and source. Install with felucca-bsp's tools/fm1_install.py (see docs/FM1_PORT.md).
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
: "${JIELI_TOOLCHAIN:?JIELI_TOOLCHAIN must point at the JieLi Linux toolchain}"
SUFFIX=""
[ "${DIAG:-0}" = 1 ] && SUFFIX="$SUFFIX-diag"
[ "${USB:-0}" = 1 ] && SUFFIX="$SUFFIX-usb"
OUT="build/firmware$SUFFIX"
NAME="fm1basic$SUFFIX"
APP="$OUT/$NAME.bin"
# Always build the selected app so a package cannot silently embed an older image.
firmware/build.sh
[ -d third_party/felucca/.git ] || tools/get_felucca.sh
[ -f third_party/felucca-bsp/tools/fm1pkg.py ] || tools/get_bsp.sh
[ -f third_party/ac79-sdk/cpu/wl82/tools/uboot.boot ] || tools/get_sdk_files.sh
( cd third_party/felucca && python3 -c "
import sys
sys.path.insert(0, 'tools')
import build
build.build_loader()
" )
PRODUCT="${PRODUCT:-FM-1_990}"
python3 third_party/felucca-bsp/tools/fm1pkg.py "$APP" third_party/felucca/build/loader/ota.bin \
    "$OUT/$NAME.fwsc" --product "$PRODUCT" --sdk third_party/ac79-sdk
python3 firmware/check_package.py "$OUT/$NAME.fwsc" "$APP" "$PRODUCT"
