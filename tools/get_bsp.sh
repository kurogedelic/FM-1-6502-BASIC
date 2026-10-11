#!/bin/sh
# Fetch hugelton/felucca-bsp (MPL-2.0; chip/usb.c is GPL-3.0 and is NOT used by this firmware)
# at the revision this project was written against. Not committed to this repository.
#   tools/get_bsp.sh [DEST]     (default: third_party/felucca-bsp)
set -eu
DEST="${1:-third_party/felucca-bsp}"
REV=c4624a5b538f1db5de2f3522e1a744cef5022c1c
if [ ! -d "$DEST/.git" ]; then
  git init -q "$DEST"
  git -C "$DEST" remote add origin https://github.com/hugelton/felucca-bsp.git
fi
git -C "$DEST" fetch -q --depth 1 origin "$REV"
git -C "$DEST" checkout -q FETCH_HEAD
echo "felucca-bsp $(git -C "$DEST" rev-parse --short HEAD) in $DEST"
