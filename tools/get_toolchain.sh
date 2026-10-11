#!/bin/sh
# Download JieLi's Linux toolchain (clang for pi32v2; Linux x86-64 only) from JieLi's package server.
#   tools/get_toolchain.sh [DEST]     (default: ~/.jieli)  ->  DEST/toolchain
# Same source Felucca's BUILDING.md uses. The toolchain is JieLi's; it is not redistributed here.
set -eu
DEST="${1:-$HOME/.jieli}"
mkdir -p "$DEST"
curl -fsSL --retry 3 https://pkgman.jieliapp.com/s/linux-toolchain | tar -xJ -C "$DEST"
TC="$(ls -d "$DEST"/jieli-linux-toolchains-* | sort | tail -1)"
ln -sfn "$(basename "$TC")" "$DEST/toolchain"
echo "JIELI_TOOLCHAIN=$DEST/toolchain ($(basename "$TC"))"
