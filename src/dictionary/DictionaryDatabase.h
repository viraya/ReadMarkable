#pragma once

#include "DictionaryTypes.h"

#include <QString>

struct sqlite3;
struct sqlite3_stmt;

class DictionaryDatabase
{
public:

    explicit DictionaryDatabase(const QString &dbPath);

    ~DictionaryDatabase();

    DictionaryDatabase(const DictionaryDatabase &) = delete;
    DictionaryDatabase &operator=(const DictionaryDatabase &) = delete;

    DictResult lookup(const QString &word) const;

    bool isAvailable() const { return m_available; }

private:

    bool prepareStatements();

    QList<DictEntry> collectEntries(sqlite3_stmt *stmt) const;

    sqlite3      *m_db          = nullptr;
    bool          m_available   = false;

    mutable sqlite3_stmt *m_exactStmt = nullptr;
    mutable sqlite3_stmt *m_ftsStmt   = nullptr;
};
