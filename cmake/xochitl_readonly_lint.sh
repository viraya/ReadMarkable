#!/bin/bash
# xochitl_readonly_lint.sh, enforce XOCH-10: ReadMarkable must never write to the xochitl directory.
#
# Usage: xochitl_readonly_lint.sh <src-dir>
#
# Strategy: grep for any source file under <src-dir> that BOTH
#   (a) contains the xochitl directory path as a string literal or comment, AND
#   (b) contains a write-style API call.
#
# Whitelisted files are permitted to mention the xochitl path for read-only purposes
# (open read-only, QDir listing, existence checks, etc.).  However, whitelisted files
# that ALSO contain write calls will still fail, the whitelist grants name-mention
# permission only, not write permission.
#
# False positives (write call and xochitl path in same file but completely unrelated
# code paths) must be resolved by the developer, not by weakening the lint.

set -euo pipefail

SRC_DIR="${1:?Usage: xochitl_readonly_lint.sh <src-dir>}"

# ---------------------------------------------------------------------------
# Whitelist, files that legitimately reference the xochitl path for read-only use.
# Add new entries here when a new read-only consumer of the xochitl directory is introduced.
# Every entry must be justified in cmake/xochitl_readonly_lint.cmake (as a comment).
# ---------------------------------------------------------------------------
WHITELIST=(
    "src/library/XochitlMetadataParser.h"      # comment: documents the directory it parses (read-only scan)
    "src/library/XochitlMetadataParser.cpp"    # never opens files for writing
    "src/library/XochitlLibrarySource.h"       # comment: documents path parameter (read-only watcher)
    "src/library/XochitlLibrarySource.cpp"     # QFileSystemWatcher + QDir listing only
    "src/library/XochitlEntry.h"               # data struct, no file I/O
    "src/library/LibraryModel.h"               # consumes XochitlEntry, no file I/O
    "src/library/LibraryModel.cpp"             # consumes XochitlEntry, no file I/O
    "src/library/CoverImageProvider.h"         # reads thumbnail path passed in from XochitlEntry
    "src/library/CoverImageProvider.cpp"       # QFile::open(ReadOnly) thumbnail reads only
    "src/main.cpp"                             # instantiates XochitlLibrarySource with path literal; no writes
)

# ---------------------------------------------------------------------------
# Write APIs we flag.  Pattern matches C++/Qt write-opening idioms.
# ---------------------------------------------------------------------------
WRITE_PATTERN='QIODevice::WriteOnly|QIODevice::Append|QSaveFile|QFile::remove\(|QDir::mkpath\(|QDir::rmdir\(|QDir::mkdir\(|::write\(|fopen\(.*"w|std::ofstream'

# ---------------------------------------------------------------------------
# 1. Find all .cpp / .h files under SRC_DIR that mention the xochitl path.
# ---------------------------------------------------------------------------
MENTIONERS=$(grep -rl -E '/home/root/\.local/share/remarkable/xochitl|remarkable/xochitl' \
    "${SRC_DIR}" --include='*.cpp' --include='*.h' 2>/dev/null || true)

if [[ -z "${MENTIONERS}" ]]; then
    echo "xochitl_readonly_lint PASSED (no files mention the xochitl directory path)"
    exit 0
fi

FAILED=0

for F in ${MENTIONERS}; do
    # Compute path relative to the project root (parent of SRC_DIR) for whitelist matching.
    REL="${F#${SRC_DIR%/src}/}"   # strips everything up to and including the project root prefix
    # Normalise Windows-style backslashes just in case.
    REL="${REL//\\//}"

    # ------------------------------------------------------------------
    # Check if this file is whitelisted.
    # ------------------------------------------------------------------
    IS_WHITELISTED=0
    for W in "${WHITELIST[@]}"; do
        if [[ "${REL}" == *"${W}"* ]]; then
            IS_WHITELISTED=1
            break
        fi
    done

    # ------------------------------------------------------------------
    # Check for write-style API calls in the file.
    # Exclude pure comment lines (// ... and * ...) to avoid matching
    # documentation that mentions write APIs in negation ("No QFile::remove").
    # ------------------------------------------------------------------
    WRITES=$(grep -nE "${WRITE_PATTERN}" "${F}" 2>/dev/null \
        | grep -vE '^\s*[0-9]+[:-][[:space:]]*(//|/\*|\*|#)' \
        || true)

    if [[ -n "${WRITES}" ]]; then
        if [[ ${IS_WHITELISTED} -eq 1 ]]; then
            echo "XOCHITL_READONLY_LINT FAIL: ${REL}"
            echo "  -> Whitelisted read-only file contains write calls (XOCH-10 violation):"
            echo "${WRITES}" | sed 's/^/     /'
        else
            echo "XOCHITL_READONLY_LINT FAIL: ${REL}"
            echo "  -> Non-whitelisted file mentions xochitl path AND contains write calls:"
            echo "${WRITES}" | sed 's/^/     /'
        fi
        FAILED=1
    else
        if [[ ${IS_WHITELISTED} -eq 0 ]]; then
            # File mentions xochitl path but is not whitelisted and has no write calls.
            # This is unexpected, flag for review so the whitelist stays accurate.
            echo "XOCHITL_READONLY_LINT WARN: ${REL}"
            echo "  -> File mentions xochitl path but is not in the whitelist."
            echo "     If this is a legitimate read-only consumer, add it to the whitelist in"
            echo "     cmake/xochitl_readonly_lint.cmake."
            # Treat as a failure so the developer must explicitly acknowledge it.
            FAILED=1
        fi
    fi
done

if [[ ${FAILED} -ne 0 ]]; then
    echo ""
    echo "xochitl_readonly_lint FAILED, ReadMarkable must never write to the xochitl directory."
    echo "See cmake/xochitl_readonly_lint.cmake for the whitelist and design rationale."
    exit 1
fi

echo "xochitl_readonly_lint PASSED"
exit 0
