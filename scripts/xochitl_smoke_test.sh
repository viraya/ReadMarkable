#!/bin/bash

set -euo pipefail

DEVICE="${1:-chiappa}"
EPUB="${2:-}"
XOCHITL_DIR="/home/root/.local/share/remarkable/xochitl"

echo "=== Xochitl Smoke Test ==="
echo "Device: root@${DEVICE}"
echo "Xochitl dir: ${XOCHITL_DIR}"
echo ""

echo "=== ADD PHASE ==="

UUID=$(ssh "root@${DEVICE}" 'cat /proc/sys/kernel/random/uuid')
echo "UUID: ${UUID}"

NOW_MS=$(ssh "root@${DEVICE}" 'echo $(($(date +%s%N) / 1000000))')

echo "Writing ${UUID}.metadata ..."
ssh "root@${DEVICE}" "cat > '${XOCHITL_DIR}/${UUID}.metadata'" <<METAEOF
{"deleted":false,"lastModified":"${NOW_MS}","lastOpened":"0","lastOpenedPage":0,"metadatamodified":true,"modified":true,"parent":"","pinned":false,"synced":false,"type":"DocumentType","version":0,"visibleName":"SMOKE TEST ${UUID}"}
METAEOF

echo "Writing ${UUID}.content ..."
ssh "root@${DEVICE}" "cat > '${XOCHITL_DIR}/${UUID}.content'" <<CONTENTEOF
{"coverPageNumber":0,"documentMetadata":{},"extraMetadata":{},"fileType":"epub","fontName":"","lineHeight":100,"margins":150,"orientation":"portrait","pageCount":0,"redirectionPageMap":[],"textAlignment":"left","textScale":1}
CONTENTEOF

echo "Writing ${UUID}.epub ..."
if [[ -n "${EPUB}" ]]; then
    scp "${EPUB}" "root@${DEVICE}:${XOCHITL_DIR}/${UUID}.epub"
    echo "  (uploaded real EPUB: ${EPUB})"
else
    ssh "root@${DEVICE}" "python3 -c \"
import zipfile, io, os
buf = io.BytesIO()
with zipfile.ZipFile(buf, 'w', zipfile.ZIP_STORED) as zf:
    zf.writestr('mimetype', 'application/epub+zip')
open('${XOCHITL_DIR}/${UUID}.epub', 'wb').write(buf.getvalue())
\"" 2>/dev/null || {
        ssh "root@${DEVICE}" "printf 'PK\x03\x04\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1e\x00\x00\x00mimetypeapplication/epub+zip' \
            > '${XOCHITL_DIR}/${UUID}.epub'"
    }
    echo "  (wrote minimal stub EPUB)"
fi

echo ""
echo "Files written to device.  ReadMarkable should detect the new book within 1 second."
echo ""
echo "OBSERVE the ReadMarkable library on the device."
echo "Book 'SMOKE TEST ${UUID}' should appear now."
printf "Press RETURN when you have confirmed the book is visible (or type the elapsed seconds): "
read -r ADD_ELAPSED
echo ""

echo "=== DELETE PHASE ==="
echo "Removing ${UUID} files and creating tombstone ..."

ssh "root@${DEVICE}" "
    rm -f '${XOCHITL_DIR}/${UUID}.metadata' \
          '${XOCHITL_DIR}/${UUID}.content' \
          '${XOCHITL_DIR}/${UUID}.epub' && \
    touch '${XOCHITL_DIR}/${UUID}.tombstone'
"

echo ""
echo "Files removed + tombstone created."
echo ""
echo "OBSERVE the ReadMarkable library on the device."
echo "Book 'SMOKE TEST ${UUID}' should disappear now."
printf "Press RETURN when you have confirmed the book is gone (or type the elapsed seconds): "
read -r DEL_ELAPSED
echo ""

echo "=== CLEANUP ==="
ssh "root@${DEVICE}" "rm -f '${XOCHITL_DIR}/${UUID}.tombstone'"
echo "Tombstone removed."
echo ""

echo "=== SUMMARY ==="
echo "UUID: ${UUID}"
echo "Add elapsed:    ${ADD_ELAPSED:-<not recorded>} s  (target: <= 1 s)"
echo "Delete elapsed: ${DEL_ELAPSED:-<not recorded>} s  (target: <= 1 s)"
echo ""
echo "Done."
