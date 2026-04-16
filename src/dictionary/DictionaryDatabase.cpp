#include "DictionaryDatabase.h"

#include <vendor/sqlite3/sqlite3.h>

#include <QDebug>
#include <QFileInfo>

static const char *kExactSql =
    "SELECT word, pos, definition, ipa, example "
    "FROM words "
    "WHERE word = ? COLLATE NOCASE "
    "LIMIT 20";

static const char *kFtsSql =
    "SELECT w.word, w.pos, w.definition, w.ipa, w.example "
    "FROM word_fts f "
    "JOIN words w ON w.rowid = f.rowid "
    "WHERE word_fts MATCH ? "
    "ORDER BY rank "
    "LIMIT 10";

DictionaryDatabase::DictionaryDatabase(const QString &dbPath)
{

    if (!QFileInfo::exists(dbPath)) {
        qInfo() << "DictionaryDatabase: database not found at" << dbPath
                << " -  dictionary unavailable (run tools/build-dictionary-db.py to build)";
        return;
    }

    const QByteArray pathBytes = dbPath.toUtf8();

    const int rc = sqlite3_open_v2(
        pathBytes.constData(),
        &m_db,
        SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX,
        nullptr
    );

    if (rc != SQLITE_OK) {
        qWarning() << "DictionaryDatabase: cannot open" << dbPath
                   << ":" << (m_db ? sqlite3_errmsg(m_db) : "unknown error");
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        return;
    }

    sqlite3_exec(m_db, "PRAGMA journal_mode=OFF;",     nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA synchronous=OFF;",      nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA cache_size=-4096;",     nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA temp_store=MEMORY;",    nullptr, nullptr, nullptr);

    if (!prepareStatements()) {
        qWarning() << "DictionaryDatabase: failed to prepare statements for" << dbPath;
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    m_available = true;
    qDebug() << "DictionaryDatabase: ready  - " << dbPath;
}

DictionaryDatabase::~DictionaryDatabase()
{
    if (m_exactStmt) {
        sqlite3_finalize(m_exactStmt);
        m_exactStmt = nullptr;
    }
    if (m_ftsStmt) {
        sqlite3_finalize(m_ftsStmt);
        m_ftsStmt = nullptr;
    }
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

DictResult DictionaryDatabase::lookup(const QString &word) const
{
    DictResult result;
    result.word = word;

    if (!m_available || !m_db) {
        return result;
    }

    const QByteArray wordBytes = word.toUtf8();

    sqlite3_reset(m_exactStmt);
    sqlite3_clear_bindings(m_exactStmt);
    sqlite3_bind_text(m_exactStmt, 1, wordBytes.constData(), -1, SQLITE_TRANSIENT);

    result.entries = collectEntries(m_exactStmt);

    if (!result.entries.isEmpty()) {
        result.found = true;
        return result;
    }

    const QByteArray ftsQuery = (word + QChar('*')).toUtf8();

    sqlite3_reset(m_ftsStmt);
    sqlite3_clear_bindings(m_ftsStmt);
    sqlite3_bind_text(m_ftsStmt, 1, ftsQuery.constData(), -1, SQLITE_TRANSIENT);

    result.entries = collectEntries(m_ftsStmt);
    result.found   = !result.entries.isEmpty();

    return result;
}

bool DictionaryDatabase::prepareStatements()
{
    if (sqlite3_prepare_v2(m_db, kExactSql, -1, &m_exactStmt, nullptr) != SQLITE_OK) {
        qWarning() << "DictionaryDatabase: prepare exact stmt failed:"
                   << sqlite3_errmsg(m_db);
        return false;
    }

    if (sqlite3_prepare_v2(m_db, kFtsSql, -1, &m_ftsStmt, nullptr) != SQLITE_OK) {
        qWarning() << "DictionaryDatabase: prepare fts stmt failed:"
                   << sqlite3_errmsg(m_db);
        return false;
    }

    return true;
}

QList<DictEntry> DictionaryDatabase::collectEntries(sqlite3_stmt *stmt) const
{
    QList<DictEntry> entries;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DictEntry e;

        if (const auto *raw = sqlite3_column_text(stmt, 0))
            e.word = QString::fromUtf8(reinterpret_cast<const char *>(raw));

        if (const auto *raw = sqlite3_column_text(stmt, 1))
            e.partOfSpeech = QString::fromUtf8(reinterpret_cast<const char *>(raw));

        if (const auto *raw = sqlite3_column_text(stmt, 2))
            e.definition = QString::fromUtf8(reinterpret_cast<const char *>(raw));

        if (const auto *raw = sqlite3_column_text(stmt, 3))
            e.ipa = QString::fromUtf8(reinterpret_cast<const char *>(raw));

        if (const auto *raw = sqlite3_column_text(stmt, 4))
            e.example = QString::fromUtf8(reinterpret_cast<const char *>(raw));

        entries.append(std::move(e));
    }

    return entries;
}
