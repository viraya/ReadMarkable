#!/usr/bin/env bash

set -euo pipefail

INSTALL_ROOT="/home/root/.local/share/readmarkable"
APPLOAD_DIR="/home/root/xovi/exthome/appload/readmarkable"
DRAFT_FILE="/opt/etc/draft/readmarkable.draft"
DRAFT_ICON_DIR="/opt/etc/draft/icons"
DRAFT_ICON="${DRAFT_ICON_DIR}/readmarkable.png"

FORCE=false
NO_LAUNCHER=false
VERBOSE=false

for arg in "$@"; do
  case "$arg" in
    --force)        FORCE=true ;;
    --no-launcher)  NO_LAUNCHER=true ;;
    --verbose)      VERBOSE=true ;;
    --purge)
      echo "[ERROR] --purge is an uninstall.sh flag; install.sh preserves user data" >&2
      exit 1
      ;;
    -h|--help)
      cat <<EOF
Usage: install.sh [--force] [--no-launcher] [--verbose]
  --force         skip device-model guard (chiappa-only by default)
  --no-launcher   skip AppLoad + Draft launcher file placement
  --verbose       log every file operation
EOF
      exit 0
      ;;
    *)
      echo "[WARN] Unknown flag: $arg"
      ;;
  esac
done

log()  { printf '\033[32m[INFO]\033[0m  %s\n' "$*"; }
warn() { printf '\033[33m[WARN]\033[0m  %s\n' "$*"; }
err()  { printf '\033[31m[ERROR]\033[0m %s\n' "$*" >&2; }
vlog() { if [[ "$VERBOSE" == "true" ]]; then printf '  %s\n' "$*"; fi }

MODEL=$(tr -d '\0' < /proc/device-tree/model 2>/dev/null || echo "")
if [[ "$FORCE" != "true" ]]; then
  if ! echo "$MODEL" | grep -qi 'chiappa\|paper pro move'; then
    err "Unsupported device: ${MODEL:-unknown}"
    err "ReadMarkable v1.1 supports only the reMarkable Paper Pro Move (chiappa)."
    err "Use --force to bypass this check (not recommended)."
    exit 2
  fi
fi
log "Device: ${MODEL:-unknown}"

FIRMWARE=$(cat /etc/version 2>/dev/null || echo "unknown")
log "Firmware: $FIRMWARE"
if [[ "$FIRMWARE" != *"5.6.75"* ]]; then
  warn "Tested on 5.6.75; installing on $FIRMWARE anyway"
fi

MANIFEST="${INSTALL_ROOT}/.install-manifest"
mkdir -p "$INSTALL_ROOT"
: > "$MANIFEST"

record() {
  echo "$1" >> "$MANIFEST"
  vlog "placed $1"
}

install_file() {
  local src="$1"
  local dst="$2"
  mkdir -p "$(dirname "$dst")"
  cp -f "$src" "$dst"
  record "$dst"
}

install_file readmarkable "${INSTALL_ROOT}/readmarkable"
chmod +x "${INSTALL_ROOT}/readmarkable"

for ttf in fonts/*.ttf; do
  install_file "$ttf" "${INSTALL_ROOT}/fonts/$(basename "$ttf")"
done

install_file VERSION "${INSTALL_ROOT}/VERSION"

install_file uninstall.sh "${INSTALL_ROOT}/uninstall.sh"
chmod +x "${INSTALL_ROOT}/uninstall.sh"

install_file launcher/external.manifest.json "${INSTALL_ROOT}/launcher/external.manifest.json"
install_file launcher/icon.png               "${INSTALL_ROOT}/launcher/icon.png"
install_file launcher/readmarkable.draft     "${INSTALL_ROOT}/launcher/readmarkable.draft"

if [[ "$NO_LAUNCHER" == "true" ]]; then
  warn "--no-launcher: skipping AppLoad + Draft placement"
else
  install_file launcher/external.manifest.json "${APPLOAD_DIR}/external.manifest.json"
  install_file launcher/icon.png               "${APPLOAD_DIR}/icon.png"

  install_file launcher/readmarkable.draft "${DRAFT_FILE}"
  install_file launcher/icon.png           "${DRAFT_ICON}"
fi

record "$MANIFEST"

log "Install complete  -  version $(cat "${INSTALL_ROOT}/VERSION")"
log ""
log "Next steps:"
log "  AppLoad users: open the AppLoad panel → press Reload"
log "  Draft/remux:   entry appears on next launcher refresh"
log "  Stock firmware: ssh root@<device> 'cd ${INSTALL_ROOT} && ./readmarkable -platform epaper'"
