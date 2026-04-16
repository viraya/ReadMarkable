# SQLite 3.45.3 Amalgamation

Bundled SQLite amalgamation for annotation persistence and dictionary storage.

**Version:** 3.45.3 (2024)
**Source:** https://www.sqlite.org/download.html  -  sqlite-amalgamation-3450300.zip

## Files

- `sqlite3.h`  -  Public API header
- `sqlite3.c`  -  Full amalgamation (~9 MB)
- `sqlite3ext.h`  -  Extension API header

## Build options (set in CMakeLists.txt)

```cmake
SQLITE_ENABLE_FTS5=1       # Full-text search (dictionary lookups)
SQLITE_THREADSAFE=1        # Serialized threading mode
SQLITE_OMIT_LOAD_EXTENSION=1  # No dynamic extension loading
```

## Updating

1. Download the new amalgamation from https://www.sqlite.org/download.html
2. Replace sqlite3.c and sqlite3.h with the new versions
3. Rebuild

## Notes

- CREngine does NOT include sqlite3.c in its static library, so no symbol
  renaming is needed. The standard sqlite3_* symbols are used directly.
- The `sqlite3_readmarkable` CMake target compiles sqlite3.c as a separate
  static library linked into the readmarkable executable.
