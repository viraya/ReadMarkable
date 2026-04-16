#!/bin/sh

set -e

XOVI_DIR="/home/root/xovi"
REBUILD_BIN="$XOVI_DIR/rebuild_hashtable"
HASHTAB_DIR="$XOVI_DIR/exthome/qt-resource-rebuilder"
HASHTAB_FILE="$HASHTAB_DIR/hashtab"
XOCHITL_BIN="/usr/bin/xochitl"
TIMEOUT_SECS=180

log() { printf '[rebuild-hashtab] %s\n' "$*"; }

[ -x "$REBUILD_BIN" ] || {
    echo "[rebuild-hashtab] $REBUILD_BIN not found or not executable" >&2
    echo "  XOVI may not be installed or this build ships without rebuild_hashtable." >&2
    exit 2
}
[ -x "$XOCHITL_BIN" ] || {
    echo "[rebuild-hashtab] $XOCHITL_BIN not found" >&2
    exit 2
}

if [ -f "$HASHTAB_FILE" ]; then
    hashtab_mtime=$(stat -c %Y "$HASHTAB_FILE" 2>/dev/null || echo 0)
    xochitl_mtime=$(stat -c %Y "$XOCHITL_BIN" 2>/dev/null || echo 0)
    if [ "$hashtab_mtime" -ge "$xochitl_mtime" ] && [ "$xochitl_mtime" -gt 0 ]; then
        log "hashtab is already current (hashtab=$hashtab_mtime xochitl=$xochitl_mtime)  -  skipping regeneration"
        exit 0
    fi
    log "stale hashtab (hashtab=$hashtab_mtime xochitl=$xochitl_mtime)  -  regenerating"
else
    log "no hashtab found  -  generating"
fi

mkdir -p "$HASHTAB_DIR"

log "running rebuild_hashtable (timeout ${TIMEOUT_SECS}s)"
if command -v timeout >/dev/null 2>&1; then
    echo | timeout "$TIMEOUT_SECS" "$REBUILD_BIN" || rc=$?
else
    echo | "$REBUILD_BIN" || rc=$?
fi
rc=${rc:-0}

if [ "$rc" -ne 0 ]; then
    echo "[rebuild-hashtab] rebuild_hashtable exited non-zero ($rc)" >&2
fi

if [ ! -s "$HASHTAB_FILE" ]; then
    echo "[rebuild-hashtab] hashtab file missing or empty after regeneration: $HASHTAB_FILE" >&2
    echo "  rebuild_hashtable exit code: $rc" >&2
    exit 3
fi
log "hashtab written: $HASHTAB_FILE ($(stat -c %s "$HASHTAB_FILE") bytes)"

if [ ! -x "$XOVI_DIR/start" ]; then
    echo "[rebuild-hashtab] $XOVI_DIR/start missing  -  restart-xochitl phase should recover via bare systemctl restart xochitl" >&2
fi

log "done"
