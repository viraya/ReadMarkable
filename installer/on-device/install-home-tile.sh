#!/bin/sh

set -e

STAGING_DIR="${1:?usage: install-home-tile.sh <staging-dir>}"
MANIFEST="$STAGING_DIR/manifest.toml"
TARGET_DIR="/home/root/xovi/exthome/qt-resource-rebuilder"
FIRMWARE_FILE="${RM_FIRMWARE_FILE:-/etc/version}"

log() { printf '[install-home-tile] %s\n' "$*"; }

[ -r "$MANIFEST" ] || {
    echo "[install-home-tile] missing manifest: $MANIFEST" >&2
    exit 2
}
[ -r "$FIRMWARE_FILE" ] || {
    echo "[install-home-tile] missing firmware file: $FIRMWARE_FILE" >&2
    exit 2
}

FIRMWARE_RAW=$(head -n 1 "$FIRMWARE_FILE" | tr -d '[:space:]')
RESOLVED="$FIRMWARE_RAW"
case "$FIRMWARE_RAW" in
    [0-9].[0-9]*) : ;;
    *)
        OS_RELEASE=/etc/os-release
        if [ -r "$OS_RELEASE" ]; then
            CAND=$(awk -F= '/^IMG_VERSION=/{
                v=$2; gsub(/"/, "", v); gsub(/[[:space:]]/, "", v); print v; exit
            }' "$OS_RELEASE")
            [ -n "$CAND" ] && RESOLVED="$CAND"
        fi
        if [ "$RESOLVED" = "$FIRMWARE_RAW" ]; then
            UPDATE_CONF=/usr/share/remarkable/update.conf
            if [ -r "$UPDATE_CONF" ]; then
                CAND=$(awk -F= '/^REMARKABLE_RELEASE_VERSION=/{
                    print $2; exit
                }' "$UPDATE_CONF" | tr -d '[:space:]')
                [ -n "$CAND" ] && RESOLVED="$CAND"
            fi
        fi
        ;;
esac

log "device firmware: raw=$FIRMWARE_RAW resolved=$RESOLVED"

ICON_PNG=$(awk -F'=' '/^icon_png[[:space:]]*=/{gsub(/[[:space:]"]/, "", $2); print $2; exit}' "$MANIFEST")

MATCH_QMD=$(awk -v want="$RESOLVED" '
    BEGIN { in_fw = 0; matched = 0; qmd = "" }
    /^\[\[firmware\]\]/ { in_fw = 1; matched = 0; qmd = ""; next }
    in_fw && /^[[:space:]]*version[[:space:]]*=/ {
        gsub(/.*=[[:space:]]*"?/, ""); gsub(/"?[[:space:]]*$/, "");
        if ($0 == want) matched = 1
    }
    in_fw && /^[[:space:]]*qmd[[:space:]]*=/ {
        gsub(/.*=[[:space:]]*"?/, ""); gsub(/"?[[:space:]]*$/, "");
        qmd = $0
        if (matched) { print qmd; exit }
    }
    /^\[\[/ && !/firmware/ { in_fw = 0 }
' "$MANIFEST")

if [ -z "$MATCH_QMD" ]; then
    log "no home-tile .qmd bundled for firmware $RESOLVED  -  AppLoad drawer tile retained (per spec Q2 silent-degrade)"
    exit 0
fi

QMD_SRC="$STAGING_DIR/$MATCH_QMD"
PNG_SRC="$STAGING_DIR/$ICON_PNG"

[ -r "$QMD_SRC" ] || { echo "[install-home-tile] manifest pointed at $MATCH_QMD but file missing at $QMD_SRC" >&2; exit 3; }
[ -r "$PNG_SRC" ] || { echo "[install-home-tile] icon png missing at $PNG_SRC" >&2; exit 3; }

mkdir -p "$TARGET_DIR"

rm -f "$TARGET_DIR/readmarkable-icon.rcc"

cp -f "$QMD_SRC" "$TARGET_DIR/$MATCH_QMD"
cp -f "$PNG_SRC" "$TARGET_DIR/$ICON_PNG"
log "installed $MATCH_QMD + $ICON_PNG for firmware $RESOLVED"

log "done"
