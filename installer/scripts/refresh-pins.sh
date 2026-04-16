#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALLER_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CONFIG="$INSTALLER_DIR/config/pinned-versions.toml"

PY=""
for candidate in python3 python; do
    if command -v "$candidate" >/dev/null 2>&1 \
        && "$candidate" --version >/dev/null 2>&1; then
        PY="$candidate"
        break
    fi
done
[ -n "$PY" ] || { echo "python required (and must actually run, not a Windows Store stub)" >&2; exit 1; }

json_extract() {
    "$PY" -c "
import json, sys
data = json.load(sys.stdin)
keys = sys.argv[1].split('.')
v = data
for k in keys:
    if k.startswith('[') and k.endswith(']'):
        v = v[int(k[1:-1])]
    else:
        v = v[k]
print(v)
" "$1"
}

asset_url() {
    "$PY" -c "
import json, sys
data = json.load(sys.stdin)
name = sys.argv[1]
for a in data.get('assets', []):
    if a['name'] == name:
        print(a['browser_download_url'])
        sys.exit(0)
sys.exit(1)
" "$1"
}

print_pin_for_release() {
    section="$1"
    repo="$2"
    asset_name="$3"

    echo "[refresh] checking $repo for $asset_name"
    api_json=$(curl -sL "https://api.github.com/repos/$repo/releases/latest")
    tag=$(echo "$api_json" | json_extract tag_name)
    url=$(echo "$api_json" | asset_url "$asset_name") || true

    if [ -z "$url" ] || [ "$url" = "null" ]; then
        echo "[refresh] $section: no asset named '$asset_name' in $tag, skipping" >&2
        return 1
    fi

    tmp=$(mktemp)
    curl -sL -o "$tmp" "$url"
    sha=$(sha256sum "$tmp" | awk '{print $1}')
    rm -f "$tmp"

    echo
    echo "[$section]"
    echo "tag    = \"$tag\""
    echo "url    = \"$url\""
    echo "sha256 = \"$sha\""
}

print_pin_for_release xovi    asivery/rm-xovi-extensions xovi-aarch64.tar.gz
print_pin_for_release appload asivery/rm-appload         appload-aarch64.zip

echo
echo "[refresh] Compare against current pins in $CONFIG and update by hand."
echo "[refresh] After updating, run installer/scripts/fetch-bundled-payload.sh to re-download + verify."
