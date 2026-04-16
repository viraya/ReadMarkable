#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALLER_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CONFIG="$INSTALLER_DIR/config/pinned-versions.toml"
PAYLOAD_DIR="$INSTALLER_DIR/payload"

[ -f "$CONFIG" ] || { echo "[fetch] missing $CONFIG" >&2; exit 1; }

mkdir -p "$PAYLOAD_DIR"

get_toml() {
    section="$1"
    key="$2"
    awk -v sect="[$section]" -v k="$key" '
        BEGIN { in_sect = 0 }
        /^\[/ { in_sect = ($0 == sect); next }
        in_sect && $0 ~ "^[[:space:]]*"k"[[:space:]]*=" {
            sub("^[[:space:]]*"k"[[:space:]]*=[[:space:]]*", "")
            gsub("^\"|\"$", "")
            print
            exit
        }
    ' "$CONFIG"
}

verify_sha256() {
    file="$1"
    expected="$2"
    actual=$(sha256sum "$file" | awk '{print $1}')
    if [ "$actual" = "$expected" ]; then
        return 0
    else
        echo "  expected: $expected"
        echo "  actual:   $actual"
        return 1
    fi
}

fetch_one() {
    label="$1"
    url="$2"
    sha="$3"
    dest="$4"

    if [ -f "$dest" ] && verify_sha256 "$dest" "$sha" >/dev/null 2>&1; then
        echo "[fetch] $label: cached, sha256 matches  -  skip"
        return 0
    fi

    echo "[fetch] $label: downloading from $url"
    curl --fail --location --silent --show-error -o "$dest.tmp" "$url"
    if ! verify_sha256 "$dest.tmp" "$sha"; then
        echo "[fetch] $label: sha256 MISMATCH  -  refusing to write" >&2
        rm -f "$dest.tmp"
        exit 2
    fi
    mv -f "$dest.tmp" "$dest"
    echo "$sha  $(basename "$dest")" > "$dest.sha256"
    echo "[fetch] $label: ok"
}

xovi_tag=$(get_toml xovi tag)
xovi_url=$(get_toml xovi url)
xovi_sha=$(get_toml xovi sha256)
fetch_one "xovi $xovi_tag" "$xovi_url" "$xovi_sha" "$PAYLOAD_DIR/xovi-aarch64.tar.gz"

appload_tag=$(get_toml appload tag)
appload_url=$(get_toml appload url)
appload_sha=$(get_toml appload sha256)
fetch_one "appload $appload_tag" "$appload_url" "$appload_sha" "$PAYLOAD_DIR/appload-aarch64.zip"

echo "[fetch] all bundled payload ready in $PAYLOAD_DIR"
ls -lh "$PAYLOAD_DIR"
