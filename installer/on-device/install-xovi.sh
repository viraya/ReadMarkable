#!/bin/sh

set -e

TARBALL="${1:?usage: install-xovi.sh <tarball> <tag> <source>}"
TAG="${2:?usage: install-xovi.sh <tarball> <tag> <source>}"
SOURCE="${3:?usage: install-xovi.sh <tarball> <tag> <source>}"

[ -f "$TARBALL" ] || { echo "[install-xovi] tarball not found: $TARBALL" >&2; exit 2; }

echo "[install-xovi] extracting $TARBALL into /home/root/"
tar -xzf "$TARBALL" -C /home/root

[ -d /home/root/xovi ]    || { echo "[install-xovi] /home/root/xovi missing after extract" >&2; exit 3; }
[ -x /home/root/xovi/start ] || { echo "[install-xovi] /home/root/xovi/start missing or not executable" >&2; exit 4; }

echo "$SOURCE" > /home/root/xovi/.install-source
echo "$TAG"    > /home/root/xovi/.install-tag

echo "[install-xovi] activating XOVI (running xovi/start)"
/home/root/xovi/start

echo "[install-xovi] done  -  tag=$TAG source=$SOURCE"
