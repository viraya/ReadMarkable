#!/bin/sh

set -e

INSTALL_ROOT="/home/root/.local/share/readmarkable"
MANIFEST="${INSTALL_ROOT}/.install-manifest"
TRIGGER_UNIT="/etc/systemd/system/xovi-trigger.service"
TRIGGER_LINK="/etc/systemd/system/multi-user.target.wants/xovi-trigger.service"
TRIGGER_DIR="/home/root/xovi-trigger"
LEGACY_UNIT="/etc/systemd/system/xovi-activate.service"
LEGACY_LINK="/etc/systemd/system/multi-user.target.wants/xovi-activate.service"
LEGACY_DIR="/home/root/xovi-activate"

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
  --remove-persistence   disable + remove xovi-trigger.service from /etc/systemd/system
                         (also removes any legacy xovi-activate artefacts)
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
    log "no manifest at $MANIFEST, nothing to walk"
fi

rmdir "${INSTALL_ROOT}/fonts" 2>/dev/null || true
rmdir "$INSTALL_ROOT"         2>/dev/null || true

APPLOAD_APP_DIR="/home/root/xovi/exthome/appload/readmarkable"
if [ -d "$APPLOAD_APP_DIR" ]; then
    rm -rf "$APPLOAD_APP_DIR"
fi

APPLOAD_SRC_FILE="/home/root/xovi/extensions.d/.appload-install-source"
if [ -f "$APPLOAD_SRC_FILE" ] && \
   [ "$(cat "$APPLOAD_SRC_FILE" 2>/dev/null)" = "readmarkable-installer" ]; then
    log "removing AppLoad extension (installed by readmarkable-installer)"
    rm -f /home/root/xovi/extensions.d/appload.so
    rm -f /home/root/xovi/extensions.d/.appload-install-source
    rm -f /home/root/xovi/extensions.d/.appload-install-tag
    rm -rf /home/root/xovi/exthome/appload
fi

HOMETILE_DIR="/home/root/xovi/exthome/qt-resource-rebuilder"
if [ -d "$HOMETILE_DIR" ]; then
    rm -f "$HOMETILE_DIR"/readmarkable-*.qmd
    rm -f "$HOMETILE_DIR"/readmarkable-icon.png
    rm -f "$HOMETILE_DIR"/readmarkable-icon.rcc
fi

if [ "$REMOVE_PERSISTENCE" = "true" ]; then
    have_trigger=false
    have_legacy=false
    if [ -f "$TRIGGER_UNIT" ] || [ -L "$TRIGGER_LINK" ] || [ -d "$TRIGGER_DIR" ]; then
        have_trigger=true
    fi
    if [ -f "$LEGACY_UNIT" ] || [ -L "$LEGACY_LINK" ] || [ -d "$LEGACY_DIR" ]; then
        have_legacy=true
    fi

    if [ "$have_trigger" = "true" ] || [ "$have_legacy" = "true" ]; then
        log "removing persistence artifacts"

        if ! awk '$2=="/"{print $4}' /proc/mounts | grep -qw rw; then
            mount -o remount,rw /
        fi

        if awk '$2=="/etc" && $3=="overlay"' /proc/mounts | grep -q .; then
            log "detected /etc overlay, tearing it down with umount -R /etc"
            umount -R /etc || log "warn: umount -R /etc failed"
        fi

        systemctl stop xovi-trigger.service 2>/dev/null || true
        systemctl disable xovi-trigger.service 2>/dev/null || true
        rm -f "$TRIGGER_UNIT" "$TRIGGER_LINK"
        rm -rf "$TRIGGER_DIR"

        systemctl disable xovi-activate.service 2>/dev/null || true
        rm -f "$LEGACY_UNIT" "$LEGACY_LINK"
        rm -rf "$LEGACY_DIR"

        rmdir /etc/systemd/system/multi-user.target.wants 2>/dev/null || true
        sync
        systemctl daemon-reload
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
