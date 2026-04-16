#pragma once

#include "SearchTypes.h"

#include <QString>
#include <QVector>

struct sqlite3;
struct sqlite3_stmt;

class SearchDatabase
{
public:

    explicit SearchDatabase(const QString &dbPath);

    ~SearchDatabase();

    SearchDatabase(const SearchDatabase &) = delete;
    SearchDatabase &operator=(const SearchDatabase &) = delete;

    bool isOpen() const { return m_db != nullptr; }

    bool hasIndex(const QString &bookSha) const;

    void clearBook(const QString &bookSha);

    void insertPage(const QString &bookSha, int pageNum,
                    const QString &chapterTitle, const QString &bodyText);

    void rebuildFtsIndex();

    QVector<SearchResult> search(const QString &bookSha,
                                 const QString &query,
                                 int limit = 50) const;

    static QString bookSha(const QString &bookPath);

private:

    bool execDDL(const char *sql);

    bool createSchema();

    bool prepareSearchStatement();

    sqlite3      *m_db          = nullptr;

    mutable sqlite3_stmt *m_searchStmt = nullptr;
};
