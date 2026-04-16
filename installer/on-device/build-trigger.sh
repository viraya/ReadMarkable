#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src/xovi-trigger"
BIN_DIR="$SCRIPT_DIR/bin"
IMAGE="muslcc/x86_64:aarch64-linux-musl"

mkdir -p "$BIN_DIR"

MOUNT_SRC="$SRC_DIR"
MOUNT_BIN="$BIN_DIR"
if command -v cygpath >/dev/null 2>&1; then
    MOUNT_SRC="$(cygpath -w "$SRC_DIR")"
    MOUNT_BIN="$(cygpath -w "$BIN_DIR")"
fi

docker run --rm \
    -v "$MOUNT_SRC":/src:ro \
    -v "$MOUNT_BIN":/out \
    "$IMAGE" \
    sh -c '
        set -eu
        gcc \
            -std=c11 -D_POSIX_C_SOURCE=200112L \
            -Os -Wall -Wextra -Wpedantic \
            -fno-exceptions -fno-stack-protector \
            -static -s \
            -o /out/xovi-trigger \
            /src/gesture.c /src/main.c
        chmod 0755 /out/xovi-trigger
    '

sha256sum "$SRC_DIR/gesture.h" "$SRC_DIR/gesture.c" "$SRC_DIR/main.c" "$SRC_DIR/Makefile" \
    | sort \
    | sha256sum \
    | awk '{print $1}' \
    > "$BIN_DIR/xovi-trigger.src.sha256"

echo "built: $BIN_DIR/xovi-trigger"
echo "hash:  $(cat "$BIN_DIR/xovi-trigger.src.sha256")"
