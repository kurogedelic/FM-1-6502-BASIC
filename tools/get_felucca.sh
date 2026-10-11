#!/bin/sh
# Fetch hugelton/Felucca (GPL-3.0-only) at a fixed revision into third_party/felucca (not committed).
# Used ONLY as a build-time tool: its proven update-loader build and package tooling are run, nothing
# from it is copied into this repository or into the firmware image.
set -eu
DEST="${1:-third_party/felucca}"
REV=a4d7ca2e5d52a4c35e2468401f00d883cadbda03
if [ ! -d "$DEST/.git" ]; then
  git init -q "$DEST"
  git -C "$DEST" remote add origin https://github.com/hugelton/Felucca.git
fi
git -C "$DEST" fetch -q --depth 1 origin "$REV"
git -C "$DEST" checkout -q FETCH_HEAD
echo "Felucca $(git -C "$DEST" rev-parse --short HEAD) in $DEST"
