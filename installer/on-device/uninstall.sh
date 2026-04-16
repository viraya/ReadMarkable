#!/bin/sh

set -e

INSTALL_ROOT="/home/root/.local/share/readmarkable"
MANIFEST="${INSTALL_ROOT}/.install-manifest"
ACTIVATE_UNIT="/etc/systemd/system/xovi-activate.service"
WANTED_LINK="/etc/systemd/system/multi-user.target.wants/xovi-activate.service"
ACTIVATE_DIR="/home/root/xovi-activate"

REMOVE_PERSISTENCE=false
REMOVE_XOVI=false
PURGE=false

for arg in "$@"; do
    case "$arg" in
        --remove-persistence) REMOVE_PERSISTENCE=true ;;
        --remove-xovi)        REMOVE_XOVI=true ;;
        --purge)              PURGE=true ;;
        -h|--help)
            cat <<EOF
Usage: uninstall.sh [--remove-persistence] [--remove-xovi] [--purge]
  --remove-persistence   disable + remove xovi-activate.service from /etc/systemd/system
                         (uses umount -R /etc on chiappa)
  --remove-xovi          rm -rf /home/root/xovi (warning: breaks any other XOVI app)
  --purge                also remove ~/.config/ReadMarkable/ user data
EOF
            exit 0
            ;;
        *) echo "[uninstall] unknown flag: $arg" >&2 ;;
    esac
done

log() { printf '[uninstall] %s\n' "$*"; }

if [ -f "$MANIFEST" ]; then
    log "removing files from manifest"
    while IFS= read -r path; do
        [ -z "$path" ] && continue
        rm -f "$path" || log "warn: could not remove $path"
    done < "$MANIFEST"
    rm -f "$MANIFEST"
else
    log "no manifest at $MANIFEST  -  nothing to walk"
fi

rmdir "${INSTALL_ROOT}/fonts" 2>/dev/null || true
rmdir "$INSTALL_ROOT"         2>/dev/null || true

APPLOAD_APP_DIR="/home/root/xovi/exthome/appload/readmarkable"
if [ -d "$APPLOAD_APP_DIR" ]; then
    rm -rf "$APPLOAD_APP_DIR"
fi

HOMETILE_DIR="/home/root/xovi/exthome/qt-resource-rebuilder"
if [ -d "$HOMETILE_DIR" ]; then
    rm -f "$HOMETILE_DIR"/readmarkable-*.qmd
    rm -f "$HOMETILE_DIR"/readmarkable-icon.png
    rm -f "$HOMETILE_DIR"/readmarkable-icon.rcc
fi

if [ "$REMOVE_PERSISTENCE" = "true" ]; then
    if [ -f "$ACTIVATE_UNIT" ] || [ -L "$WANTED_LINK" ] || [ -d "$ACTIVATE_DIR" ]; then
        log "removing r4 persistence artifacts"

        if ! awk '$2=="/"{print $4}' /proc/mounts | grep -qw rw; then
            mount -o remount,rw /
        fi

        if awk '$2=="/etc" && $3=="overlay"' /proc/mounts | grep -q .; then
            log "detected /etc overlay  -  tearing it down with umount -R /etc"
            umount -R /etc || log "warn: umount -R /etc failed"
        fi

        systemctl disable xovi-activate.service 2>/dev/null || true
        rm -f "$ACTIVATE_UNIT"
        rm -f "$WANTED_LINK"
        rmdir /etc/systemd/system/multi-user.target.wants 2>/dev/null || true
        sync
        systemctl daemon-reload

        rm -rf "$ACTIVATE_DIR"
    fi
fi

if [ "$REMOVE_XOVI" = "true" ]; then
    if [ -x /home/root/xovi/stock ]; then
        log "deactivating XOVI (xovi/stock)"
        /home/root/xovi/stock || true
    fi
    log "removing /home/root/xovi"
    rm -rf /home/root/xovi
fi

if [ "$PURGE" = "true" ]; then
    log "purging ~/.config/ReadMarkable/"
    rm -rf /home/root/.config/ReadMarkable
fi

log "done"
