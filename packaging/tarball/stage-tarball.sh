#!/usr/bin/env bash

set -euo pipefail

VERSION="${1:?usage: stage-tarball.sh <version>}"
STAGING="readmarkable-${VERSION}-chiappa"
TARBALL="${STAGING}.tar.gz"

rm -rf "${STAGING}" "${TARBALL}" "${TARBALL}.sha256"
mkdir -p "${STAGING}/fonts" "${STAGING}/launcher"

cp build/bin/readmarkable "${STAGING}/readmarkable"
echo "${VERSION}" > "${STAGING}/VERSION"

cp fonts/*.ttf "${STAGING}/fonts/"
FONT_COUNT=$(ls "${STAGING}/fonts/"*.ttf | wc -l)
if [[ "${FONT_COUNT}" -ne 24 ]]; then
  echo "[ERROR] expected 24 font TTFs (8 families x 3 weights), found ${FONT_COUNT}" >&2
  exit 1
fi

cp packaging/appload/external.manifest.json "${STAGING}/launcher/"
cp packaging/appload/icon.png               "${STAGING}/launcher/"
cp packaging/draft/readmarkable.draft       "${STAGING}/launcher/"

cp install.sh   "${STAGING}/install.sh"
cp uninstall.sh "${STAGING}/uninstall.sh"
chmod +x "${STAGING}/install.sh" "${STAGING}/uninstall.sh"

tar --sort=name \
    --owner=0 --group=0 --numeric-owner \
    --mtime='2026-01-01 00:00:00 UTC' \
    -cf - "${STAGING}" | gzip -n > "${TARBALL}"

sha256sum "${TARBALL}" > "${TARBALL}.sha256"

echo "[OK] Staged ${TARBALL}"
ls -lh "${TARBALL}" "${TARBALL}.sha256"
