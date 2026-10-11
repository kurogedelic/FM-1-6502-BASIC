#!/bin/sh
# Fetch only the three JieLi AC79 SDK files that package building needs (Apache-2.0, from JieLi's
# repository at the revision felucca-bsp was tested with) and verify their SHA-256.
# Uses a blob-less sparse checkout, so it is much smaller than felucca-bsp's tools/get_sdk.sh.
#   tools/get_sdk_files.sh [DEST]   (default: third_party/ac79-sdk)   then: export AC79_SDK=DEST
set -eu
DEST="${1:-third_party/ac79-sdk}"
URL="${SDK_URL:-https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK}"   # (the GitHub mirror felucca-bsp names no longer exists)
REV=AC79NN_SDK_V1.2.1_2023-12-13
mkdir -p "$DEST"
cd "$DEST"
[ -d .git ] || git init -q .
git fetch -q --depth 1 --filter=blob:none "$URL" "refs/tags/$REV:refs/tags/$REV" || git fetch -q --depth 1 "$URL" "refs/tags/$REV:refs/tags/$REV"
git sparse-checkout init --no-cone
git sparse-checkout set /LICENSE /cpu/wl82/tools/uboot.boot /cpu/wl82/tools/cfg_tool.bin /cpu/wl82/tools/cfg/eq_cfg_hw.bin
git checkout -q "$REV"
check() { echo "$2  $1" > .sum; (sha256sum -c .sum >/dev/null 2>&1 || shasum -a 256 -c .sum >/dev/null 2>&1) || { echo "get_sdk_files: $1 is not the file this project was tested with" >&2; exit 1; }; }
check cpu/wl82/tools/uboot.boot        4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4
check cpu/wl82/tools/cfg_tool.bin      276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b
check cpu/wl82/tools/cfg/eq_cfg_hw.bin 41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d
rm -f .sum
echo "AC79_SDK=$DEST ($(git rev-parse --short HEAD), Apache-2.0: see $DEST/LICENSE)"
