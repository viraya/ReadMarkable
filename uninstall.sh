#!/usr/bin/env bash

set -euo pipefail

INSTALL_ROOT="/home/root/.local/share/readmarkable"
MANIFEST="${INSTALL_ROOT}/.install-manifest"
APPLOAD_DIR="/home/root/xovi/exthome/appload/readmarkable"
QSETTINGS_DIR="/home/root/.config/ReadMarkable"

PURGE=false
VERBOSE=false

for arg in "$@"; do
  case "$arg" in
    --purge)   PURGE=true ;;
    --verbose) VERBOSE=true ;;
    -h|--help)
      cat <<EOF
Usage: uninstall.sh [--purge] [--verbose]
  --purge    also remove ~/.config/ReadMarkable (positions, highlights, preferences)
  --verbose  log every file removal
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
vlog() { if [[ "$VERBOSE" == "true" ]]; then printf '  %s\n' "$*"; fi }

if [[ ! -f "$MANIFEST" ]]; then
  warn "No install manifest found at $MANIFEST, nothing to uninstall"
  warn "(If this is wrong, remove $INSTALL_ROOT manually.)"
  exit 0
fi

SELF="$(readlink -f "$0" 2>/dev/null || realpath "$0" 2>/dev/null || echo "$0")"

tac "$MANIFEST" | while IFS= read -r f; do
  [[ -z "$f" ]] && continue
  if [[ "$f" == "$SELF" ]]; then
    continue
  fi
  if [[ -e "$f" || -L "$f" ]]; then
    rm -f "$f"
    vlog "removed $f"
  fi
done

for d in "$APPLOAD_DIR" "${INSTALL_ROOT}/fonts" "${INSTALL_ROOT}/launcher" "/opt/etc/draft/icons"; do
  if [[ -d "$d" ]]; then
    rmdir "$d" 2>/dev/null && vlog "rmdir $d" || true
  fi
done

if [[ "$PURGE" == "true" ]]; then
  if [[ -d "$QSETTINGS_DIR" ]]; then
    rm -rf "$QSETTINGS_DIR"
    log "--purge: removed $QSETTINGS_DIR"
  fi
fi

if [[ "$PURGE" == "true" ]]; then
  log "Uninstall complete (--purge: user data also removed)"
else
  log "Uninstall complete"
fi

if [[ -f "$SELF" ]]; then
  exec /bin/sh -c "rm -f '$SELF'; rmdir '$INSTALL_ROOT' 2>/dev/null || true"
fi
rmdir "$INSTALL_ROOT" 2>/dev/null || true
