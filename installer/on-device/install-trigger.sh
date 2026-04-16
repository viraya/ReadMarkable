#!/bin/sh

set -e

STAGING_DIR="${1:?usage: install-trigger.sh <staging-dir>}"
UNIT_SRC="$STAGING_DIR/xovi-trigger.service"
BIN_SRC="$STAGING_DIR/xovi-trigger"

[ -r "$UNIT_SRC" ] || { echo "[install-trigger] missing: $UNIT_SRC" >&2; exit 2; }
[ -r "$BIN_SRC" ]  || { echo "[install-trigger] missing: $BIN_SRC"  >&2; exit 2; }

UNIT_DST="/etc/systemd/system/xovi-trigger.service"
WANTED_LINK="/etc/systemd/system/multi-user.target.wants/xovi-trigger.service"
TRIGGER_DIR="/home/root/xovi-trigger"
TRIGGER_DST="$TRIGGER_DIR/xovi-trigger"

LEGACY_UNIT="/etc/systemd/system/xovi-activate.service"
LEGACY_WANT="/etc/systemd/system/multi-user.target.wants/xovi-activate.service"
LEGACY_DIR="/home/root/xovi-activate"

log() { printf '[install-trigger] %s\n' "$*"; }

if awk '$2=="/"{print $4}' /proc/mounts | grep -qw rw; then
    :
else
    log "remounting rootfs rw"
    mount -o remount,rw /
fi

if awk '$2=="/etc" && $3=="overlay"' /proc/mounts | grep -q .; then
    log "detected /etc overlay (chiappa firmware), tearing it down with umount -R /etc"
    umount -R /etc || log "warn: umount -R /etc failed; continuing (writes may not persist)"
fi

if [ -f "$LEGACY_UNIT" ] || [ -L "$LEGACY_WANT" ] || [ -d "$LEGACY_DIR" ]; then
    log "cleaning up legacy xovi-activate autostart"
    systemctl disable xovi-activate.service 2>/dev/null || true
    rm -f "$LEGACY_UNIT" "$LEGACY_WANT"
    rm -rf "$LEGACY_DIR"
fi

mkdir -p "$TRIGGER_DIR"
cp -f "$BIN_SRC" "$TRIGGER_DST"
chmod 0755 "$TRIGGER_DST"
log "staged $TRIGGER_DST"

mkdir -p /etc/systemd/system
cp -f "$UNIT_SRC" "$UNIT_DST"
sync
log "wrote $UNIT_DST"

systemctl daemon-reload
systemctl enable xovi-trigger.service
log "enabled xovi-trigger.service (symlink: $WANTED_LINK)"

systemctl start xovi-trigger.service 2>/dev/null || log "warn: start failed (unit may already be running, or systemd not running under mock)"

log "done  listener is active now AND on reboot"
