#!/usr/bin/env bash

set -euo pipefail

APP_BUNDLE="${1:?usage: sign-macos.sh <path-to-ReadMarkable Installer.app>}"
VERSION="${2:?usage: sign-macos.sh <app> <version>}"

if [[ -z "${APPLE_DEV_ID_CERT_BASE64:-}" ]]; then
    echo "[sign] APPLE_DEV_ID_CERT_BASE64 not set  -  skipping signing" >&2
    exit 2
fi

KEYCHAIN="$HOME/signing.keychain-db"
KEYCHAIN_PASSWORD="$(openssl rand -hex 16)"
echo "[sign] creating temp keychain"
security create-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN"
security set-keychain-settings -lut 21600 "$KEYCHAIN"
security unlock-keychain -p "$KEYCHAIN_PASSWORD" "$KEYCHAIN"
security list-keychains -d user -s "$KEYCHAIN" $(security list-keychains -d user | tr -d '"')

P12="$(mktemp -t cert).p12"
echo "$APPLE_DEV_ID_CERT_BASE64" | base64 -d > "$P12"
security import "$P12" -k "$KEYCHAIN" -P "$APPLE_DEV_ID_CERT_PASSWORD" -T /usr/bin/codesign
security set-key-partition-list -S apple-tool:,apple: -s -k "$KEYCHAIN_PASSWORD" "$KEYCHAIN"
rm -f "$P12"

IDENTITY="Developer ID Application"
echo "[sign] codesigning $APP_BUNDLE"
codesign --deep --force --options runtime \
    --entitlements "$(dirname "$0")/entitlements.plist" \
    --sign "$IDENTITY" "$APP_BUNDLE"
codesign --verify --strict --verbose=2 "$APP_BUNDLE"

ZIP="${APP_BUNDLE%.app}.zip"
echo "[sign] zipping for notarytool"
ditto -c -k --sequesterRsrc --keepParent "$APP_BUNDLE" "$ZIP"

echo "[sign] submitting to notarytool"
xcrun notarytool submit "$ZIP" \
    --apple-id "$APPLE_ID" --team-id "$TEAM_ID" \
    --password "$APP_PASSWORD" --wait

echo "[sign] stapling notarization ticket"
xcrun stapler staple "$APP_BUNDLE"

DMG="ReadMarkableInstaller-${VERSION}.dmg"
echo "[sign] building $DMG"
hdiutil create -volname "ReadMarkable Installer" \
    -srcfolder "$APP_BUNDLE" -ov -format UDZO "$DMG"
codesign --sign "$IDENTITY" "$DMG"

shasum -a 256 "$DMG" | awk '{print $1"  "$2}' > "$DMG.sha256"

echo "[sign] OK  -  $DMG ready"
