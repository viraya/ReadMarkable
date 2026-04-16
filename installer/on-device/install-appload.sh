#!/bin/sh

set -e

ZIP="${1:?usage: install-appload.sh <zip> <tag> <source>}"
TAG="${2:?usage: install-appload.sh <zip> <tag> <source>}"
SOURCE="${3:?usage: install-appload.sh <zip> <tag> <source>}"

[ -f "$ZIP" ] || { echo "[install-appload] zip not found: $ZIP" >&2; exit 2; }
[ -d /home/root/xovi ] || { echo "[install-appload] /home/root/xovi missing, install XOVI first" >&2; exit 3; }

EXT_DIR="/home/root/xovi/extensions.d"
mkdir -p "$EXT_DIR"

echo "[install-appload] unzipping $ZIP into $EXT_DIR"
unzip -oq "$ZIP" -d "$EXT_DIR"

[ -f "$EXT_DIR/appload.so" ] || { echo "[install-appload] appload.so not present after unzip" >&2; exit 4; }

echo "$SOURCE" > "$EXT_DIR/.appload-install-source"
echo "$TAG"    > "$EXT_DIR/.appload-install-tag"

echo "[install-appload] done, tag=$TAG source=$SOURCE"
