#!/bin/sh

set -e

STAGING_DIR="${1:?usage: install-persistence.sh <staging-dir>}"
UNIT_SRC="$STAGING_DIR/xovi-activate.service"
ACTIVATE_SRC="$STAGING_DIR/xovi-activate.sh"

[ -r "$UNIT_SRC" ]     || { echo "[install-persistence] missing: $UNIT_SRC" >&2; exit 2; }
[ -r "$ACTIVATE_SRC" ] || { echo "[install-persistence] missing: $ACTIVATE_SRC" >&2; exit 2; }

UNIT_DST="/etc/systemd/system/xovi-activate.service"
WANTED_LINK="/etc/systemd/system/multi-user.target.wants/xovi-activate.service"
ACTIVATE_DIR="/home/root/xovi-activate"
ACTIVATE_DST="$ACTIVATE_DIR/xovi-activate.sh"

log() { printf '[install-persistence] %s\n' "$*"; }

mkdir -p "$ACTIVATE_DIR"
cp -f "$ACTIVATE_SRC" "$ACTIVATE_DST"
chmod +x "$ACTIVATE_DST"
log "staged $ACTIVATE_DST"

ORIG_RW=false
if awk '$2=="/"{print $4}' /proc/mounts | grep -qw rw; then
    ORIG_RW=true
else
    log "remounting rootfs rw"
    mount -o remount,rw /
fi

if awk '$2=="/etc" && $3=="overlay"' /proc/mounts | grep -q .; then
    log "detected /etc overlay (chiappa firmware)  -  tearing it down with umount -R /etc"
    umount -R /etc || log "warn: umount -R /etc failed; continuing (writes may not persist)"
fi

mkdir -p /etc/systemd/system
cp -f "$UNIT_SRC" "$UNIT_DST"
sync

log "wrote $UNIT_DST"

mkdir -p /run
touch /run/xovi-activated

systemctl daemon-reload

systemctl enable xovi-activate.service
log "enabled xovi-activate.service (symlink: $WANTED_LINK)"

log "done  -  persistence active now AND on reboot"
