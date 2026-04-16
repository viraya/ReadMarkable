#!/usr/bin/env python3
"""
build-dictionary-db.py  -  Offline dictionary database builder for ReadMarkable

Builds dict-en.db from Open English Wordnet (OEWN) data.
The output is an FTS5-indexed SQLite database consumed by DictionaryDatabase.h.

Usage:
    python3 tools/build-dictionary-db.py
    python3 tools/build-dictionary-db.py --output data/dict-en.db
    python3 tools/build-dictionary-db.py --input oewn-2024.db --output data/dict-en.db

If --input is not provided the script downloads the OEWN 2024 SQLite release
from GitHub automatically (requires internet access on the developer's machine).

The generated database is deployed to the device at:
    /home/root/.readmarkable/data/dict-en.db

Schema produced:
    CREATE TABLE words (
        word       TEXT NOT NULL,
        pos        TEXT NOT NULL,
        definition TEXT NOT NULL,
        ipa        TEXT DEFAULT '',
        example    TEXT DEFAULT ''
    );
    CREATE VIRTUAL TABLE word_fts USING fts5(word, content=words, content_rowid=rowid);
    CREATE INDEX idx_words_word ON words(word COLLATE NOCASE);

OEWN part-of-speech codes are mapped as follows:
    n -> noun   v -> verb   a -> adj   r -> adv   s -> adj (satellite)
"""

import argparse
import os
import sys
import sqlite3
import tempfile
import urllib.request
import urllib.error
import shutil
import time

# ─── OEWN release URL ─────────────────────────────────────────────────────────

# OEWN 2024 SQLite release on GitHub.
# Check https://github.com/globalwordnet/english-wordnet/releases for newer versions.
OEWN_SQLITE_URL = (
    "https://github.com/globalwordnet/english-wordnet/releases/download/"
    "2024-edition/english-wordnet-2024.db"
)

# Fallback: OEWN YAML/LMF releases can also be used but require more parsing.
# The SQLite release is the simplest input format.

# ─── POS mapping ─────────────────────────────────────────────────────────────

POS_MAP = {
    "n": "noun",
    "v": "verb",
    "a": "adj",
    "s": "adj",      # satellite adjective (similar-to)
    "r": "adv",
}

# ─── Target schema DDL ───────────────────────────────────────────────────────

TARGET_DDL = """
CREATE TABLE words (
    word       TEXT NOT NULL,
    pos        TEXT NOT NULL,
    definition TEXT NOT NULL,
    ipa        TEXT DEFAULT '',
    example    TEXT DEFAULT ''
);

CREATE INDEX idx_words_word ON words(word COLLATE NOCASE);

CREATE VIRTUAL TABLE word_fts USING fts5(word, content=words, content_rowid=rowid);
"""

# ─── Helpers ─────────────────────────────────────────────────────────────────

def download_oewn(dest_path: str) -> None:
    """Download the OEWN SQLite database to dest_path, showing progress."""
    print(f"Downloading OEWN 2024 from:\n  {OEWN_SQLITE_URL}")
    print(f"Destination: {dest_path}")

    try:
        with urllib.request.urlopen(OEWN_SQLITE_URL, timeout=120) as response:
            total = int(response.headers.get("Content-Length", 0))
            downloaded = 0
            chunk_size = 1024 * 64  # 64 KB chunks

            with open(dest_path, "wb") as out:
                while True:
                    chunk = response.read(chunk_size)
                    if not chunk:
                        break
                    out.write(chunk)
                    downloaded += len(chunk)
                    if total > 0:
                        pct = downloaded / total * 100
                        print(f"\r  {downloaded // 1024:,} KB / {total // 1024:,} KB  ({pct:.1f}%)",
                              end="", flush=True)
        print()  # newline after progress
        print(f"Download complete: {os.path.getsize(dest_path) // 1024:,} KB")

    except urllib.error.URLError as exc:
        print(f"\nERROR: Download failed: {exc}", file=sys.stderr)
        print(
            "\nAlternative: Download the OEWN SQLite file manually from:\n"
            f"  {OEWN_SQLITE_URL}\n"
            "Then pass it with --input:\n"
            "  python3 tools/build-dictionary-db.py --input /path/to/english-wordnet-2024.db",
            file=sys.stderr,
        )
        sys.exit(1)


def detect_oewn_schema(conn: sqlite3.Connection) -> str:
    """
    Detect the OEWN SQLite schema variant and return the appropriate
    extraction query.

    OEWN 2024 SQLite schema has these relevant tables:
      - entry (id, lemma, pos)
      - sense (id, synset, entry, ...)
      - synset (id, pos, ili, definition, ...)
      - example (synset, text)

    Returns the variant name: "oewn2024" or raises if unrecognised.
    """
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = {row[0] for row in cursor.fetchall()}

    if {"entry", "sense", "synset"}.issubset(tables):
        return "oewn2024"

    # Some OEWN SQLite builds flatten into a single 'words' table.
    if "words" in tables:
        return "flat"

    raise RuntimeError(
        f"Unrecognised OEWN schema. Tables found: {sorted(tables)}\n"
        "Ensure the input is a valid OEWN SQLite release.\n"
        "Download from: https://github.com/globalwordnet/english-wordnet/releases"
    )


def extract_oewn2024(src_conn: sqlite3.Connection):
    """
    Yield (word, pos, definition, ipa, example) tuples from an OEWN 2024 db.

    Joins entry → sense → synset, collecting definition and the first example.
    IPA is not stored in OEWN  -  that column is left empty for now.
    A future enhancement could add IPA from CMU Pronouncing Dictionary or
    the WikiPron dataset.
    """
    query = """
    SELECT
        e.lemma                              AS word,
        e.pos                                AS pos_code,
        s.definition                         AS definition,
        ''                                   AS ipa,
        COALESCE(
            (SELECT ex.text
             FROM   example ex
             WHERE  ex.synset = s.id
             LIMIT  1),
            ''
        )                                    AS example
    FROM  entry e
    JOIN  sense se ON se.entry = e.id
    JOIN  synset s  ON s.id     = se.synset
    WHERE e.lemma IS NOT NULL
      AND e.lemma  != ''
      AND s.definition IS NOT NULL
      AND s.definition != ''
    ORDER BY e.lemma, e.pos
    """
    cursor = src_conn.cursor()
    cursor.execute(query)
    return cursor  # iterate lazily


def extract_flat(src_conn: sqlite3.Connection):
    """
    Yield (word, pos, definition, ipa, example) tuples from a flat 'words' table.

    Assumes the flat schema has at least word and definition columns.
    Handles optional pos, ipa, example columns gracefully.
    """
    cursor = src_conn.cursor()
    cursor.execute("PRAGMA table_info(words)")
    columns = {row[1] for row in cursor.fetchall()}

    col_pos    = "pos"        if "pos"        in columns else "'n'"
    col_ipa    = "ipa"        if "ipa"        in columns else "''"
    col_example = "example"  if "example"    in columns else "''"

    query = f"SELECT word, {col_pos}, definition, {col_ipa}, {col_example} FROM words"
    cursor.execute(query)
    return cursor


def map_pos(pos_code: str) -> str:
    """Map a raw POS code to a human-readable string."""
    return POS_MAP.get(str(pos_code).lower().strip(), str(pos_code))


def build_database(src_path: str, out_path: str) -> None:
    """
    Read OEWN data from src_path and write the ReadMarkable dictionary to out_path.
    """
    print(f"\nReading source: {src_path}")

    # Open source db read-only.
    src_conn = sqlite3.connect(f"file:{src_path}?mode=ro", uri=True)
    src_conn.row_factory = sqlite3.Row

    # Detect schema variant.
    schema = detect_oewn_schema(src_conn)
    print(f"Detected OEWN schema variant: {schema}")

    # Remove existing output file so we start fresh.
    if os.path.exists(out_path):
        os.remove(out_path)
        print(f"Removed existing output: {out_path}")

    # Create output database.
    out_conn = sqlite3.connect(out_path)
    out_conn.execute("PRAGMA journal_mode=WAL")
    out_conn.execute("PRAGMA synchronous=NORMAL")
    out_conn.execute("PRAGMA cache_size=-32768")  # 32 MB during build

    # Create schema.
    for stmt in TARGET_DDL.strip().split(";"):
        stmt = stmt.strip()
        if stmt:
            out_conn.execute(stmt)
    out_conn.commit()
    print("Target schema created.")

    # Extract and insert rows.
    print("Extracting and inserting entries…")
    start = time.monotonic()
    count = 0

    if schema == "oewn2024":
        rows = extract_oewn2024(src_conn)
    else:
        rows = extract_flat(src_conn)

    batch = []
    BATCH_SIZE = 10_000

    for row in rows:
        word, pos_code, definition, ipa, example = (
            row[0], row[1], row[2], row[3], row[4]
        )

        if not word or not definition:
            continue

        batch.append((
            str(word).strip(),
            map_pos(pos_code),
            str(definition).strip(),
            str(ipa).strip() if ipa else "",
            str(example).strip() if example else "",
        ))

        count += 1

        if len(batch) >= BATCH_SIZE:
            out_conn.executemany(
                "INSERT INTO words (word, pos, definition, ipa, example) VALUES (?,?,?,?,?)",
                batch,
            )
            out_conn.commit()
            batch.clear()
            elapsed = time.monotonic() - start
            print(f"\r  {count:,} entries inserted  ({elapsed:.1f}s)", end="", flush=True)

    # Final batch.
    if batch:
        out_conn.executemany(
            "INSERT INTO words (word, pos, definition, ipa, example) VALUES (?,?,?,?,?)",
            batch,
        )
        out_conn.commit()

    elapsed = time.monotonic() - start
    print(f"\r  {count:,} entries inserted  ({elapsed:.1f}s)")

    src_conn.close()

    # Build FTS5 index.
    print("Building FTS5 index (may take 30–60 seconds)…")
    t_fts = time.monotonic()
    out_conn.execute("INSERT INTO word_fts(word_fts) VALUES('rebuild')")
    out_conn.commit()
    print(f"FTS5 index built in {time.monotonic() - t_fts:.1f}s")

    # VACUUM to compact the file.
    print("Running VACUUM to minimise file size…")
    t_vac = time.monotonic()
    out_conn.execute("VACUUM")
    out_conn.commit()
    print(f"VACUUM complete in {time.monotonic() - t_vac:.1f}s")

    out_conn.close()

    # Print summary.
    size_kb = os.path.getsize(out_path) // 1024
    total_elapsed = time.monotonic() - start
    print(f"\nDone!")
    print(f"  Entries  : {count:,}")
    print(f"  File size: {size_kb:,} KB  ({out_path})")
    print(f"  Total time: {total_elapsed:.1f}s")
    print(
        f"\nDeploy to device with:\n"
        f"  scp {out_path} root@<device-ip>:/home/root/.readmarkable/data/dict-en.db"
    )


# ─── Entry point ─────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Build the ReadMarkable offline dictionary database from OEWN data.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--output", "-o",
        default=os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "data", "dict-en.db"
        ),
        metavar="PATH",
        help="Output .db path (default: data/dict-en.db relative to project root)",
    )
    parser.add_argument(
        "--input", "-i",
        default=None,
        metavar="PATH",
        help="Path to a pre-downloaded OEWN SQLite file (skips download)",
    )
    args = parser.parse_args()

    out_dir = os.path.dirname(os.path.abspath(args.output))
    os.makedirs(out_dir, exist_ok=True)

    if args.input:
        # Use supplied input file directly.
        if not os.path.exists(args.input):
            print(f"ERROR: Input file not found: {args.input}", file=sys.stderr)
            sys.exit(1)
        build_database(args.input, args.output)
    else:
        # Download OEWN to a temp file, then build.
        with tempfile.TemporaryDirectory() as tmp_dir:
            tmp_db = os.path.join(tmp_dir, "oewn-download.db")
            download_oewn(tmp_db)
            build_database(tmp_db, args.output)


if __name__ == "__main__":
    main()
