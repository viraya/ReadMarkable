#!/bin/sh

set -e

INSTALL_ROOT="/home/root/.local/share/readmarkable"
APPLOAD_APP_DIR="/home/root/xovi/exthome/appload/readmarkable"

log()  { printf '[install-rm] %s\n' "$*"; }

MANIFEST="${INSTALL_ROOT}/.install-manifest"
mkdir -p "$INSTALL_ROOT"
: > "$MANIFEST"

record() {
    echo "$1" >> "$MANIFEST"
}

install_file() {
    src="$1"
    dst="$2"
    mkdir -p "$(dirname "$dst")"
    cp -f "$src" "$dst"
    record "$dst"
}

install_file readmarkable "${INSTALL_ROOT}/readmarkable"
chmod +x "${INSTALL_ROOT}/readmarkable"
install_file VERSION "${INSTALL_ROOT}/VERSION"

for ttf in fonts/*.ttf; do
    [ -e "$ttf" ] || continue
    install_file "$ttf" "${INSTALL_ROOT}/fonts/$(basename "$ttf")"
done

install_file launcher/external.manifest.json "${APPLOAD_APP_DIR}/external.manifest.json"
install_file launcher/icon.png               "${APPLOAD_APP_DIR}/icon.png"

record "$MANIFEST"

log "install complete  -  version $(cat "${INSTALL_ROOT}/VERSION")"
