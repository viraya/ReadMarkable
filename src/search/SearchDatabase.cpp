#include "SearchDatabase.h"

#include <vendor/sqlite3/sqlite3.h>

#include <QCryptographicHash>
#include <QDebug>
#include <QFileInfo>

static const char *kCreateBookPages = R"sql(
CREATE TABLE IF NOT EXISTS book_pages (
    book_sha      TEXT    NOT NULL,
    page_num      INTEGER NOT NULL,
    chapter_title TEXT    NOT NULL DEFAULT '',
    body_text     TEXT    NOT NULL,
    PRIMARY KEY (book_sha, page_num)
);
)sql";

static const char *kCreatePageFts = R"sql(
CREATE VIRTUAL TABLE IF NOT EXISTS page_fts
    USING fts5(body_text, content=book_pages, content_rowid=rowid);
)sql";

static const char *kCreateBookIndex = R"sql(
CREATE INDEX IF NOT EXISTS idx_bp_book ON book_pages(book_sha);
)sql";

static const char *kSearchSql =
    "SELECT bp.page_num, bp.chapter_title, "
    "       snippet(page_fts, 0, '**', '**', '...', 15) AS snip "
    "FROM page_fts "
    "JOIN book_pages bp ON bp.rowid = page_fts.rowid "
    "WHERE page_fts MATCH ? AND bp.book_sha = ? "
    "ORDER BY rank "
    "LIMIT ?";

SearchDatabase::SearchDatabase(const QString &dbPath)
{
    const QByteArray pathBytes = dbPath.toUtf8();
    const int rc = sqlite3_open(pathBytes.constData(), &m_db);
    if (rc != SQLITE_OK) {
        qCritical() << "SearchDatabase: cannot open database"
                    << dbPath << ":" << sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;",   nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA cache_size=-4096;",   nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA temp_store=MEMORY;",  nullptr, nullptr, nullptr);

    if (!createSchema()) {
        qCritical() << "SearchDatabase: schema creation failed for" << dbPath;
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    if (!prepareSearchStatement()) {
        qWarning() << "SearchDatabase: failed to prepare search statement";

    }

    qDebug() << "SearchDatabase: ready -" << dbPath;
}

SearchDatabase::~SearchDatabase()
{
    if (m_searchStmt) {
        sqlite3_finalize(m_searchStmt);
        m_searchStmt = nullptr;
    }
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SearchDatabase::hasIndex(const QString &bookSha) const
{
    if (!m_db) return false;

    const char *sql = "SELECT COUNT(*) FROM book_pages WHERE book_sha = ?";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "SearchDatabase::hasIndex prepare error:" << sqlite3_errmsg(m_db);
        return false;
    }

    const QByteArray shaBytes = bookSha.toUtf8();
    sqlite3_bind_text(stmt, 1, shaBytes.constData(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        found = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return found;
}

void SearchDatabase::clearBook(const QString &bookSha)
{
    if (!m_db) return;

    const char *sql = "DELETE FROM book_pages WHERE book_sha = ?";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "SearchDatabase::clearBook prepare error:" << sqlite3_errmsg(m_db);
        return;
    }

    const QByteArray shaBytes = bookSha.toUtf8();
    sqlite3_bind_text(stmt, 1, shaBytes.constData(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        qWarning() << "SearchDatabase::clearBook step error:" << sqlite3_errmsg(m_db);
    } else {
        qDebug() << "SearchDatabase: cleared index for book" << bookSha.left(12) << "...";
    }
}

void SearchDatabase::insertPage(const QString &bookSha, int pageNum,
                                const QString &chapterTitle, const QString &bodyText)
{
    if (!m_db) return;

    const char *sql =
        "INSERT OR REPLACE INTO book_pages (book_sha, page_num, chapter_title, body_text) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "SearchDatabase::insertPage prepare error:" << sqlite3_errmsg(m_db);
        return;
    }

    const QByteArray shaBytes   = bookSha.toUtf8();
    const QByteArray titleBytes = chapterTitle.toUtf8();
    const QByteArray bodyBytes  = bodyText.toUtf8();

    sqlite3_bind_text(stmt, 1, shaBytes.constData(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 2, pageNum);
    sqlite3_bind_text(stmt, 3, titleBytes.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, bodyBytes.constData(),  -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        qWarning() << "SearchDatabase::insertPage step error for page" << pageNum
                   << ":" << sqlite3_errmsg(m_db);
    }
}

void SearchDatabase::rebuildFtsIndex()
{
    if (!m_db) return;

    const char *sql = "INSERT INTO page_fts(page_fts) VALUES('rebuild')";
    char *errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qWarning() << "SearchDatabase::rebuildFtsIndex error:" << errMsg;
        sqlite3_free(errMsg);
    } else {
        qDebug() << "SearchDatabase: FTS5 index rebuilt";
    }
}

QVector<SearchResult> SearchDatabase::search(const QString &bookSha,
                                              const QString &query,
                                              int limit) const
{
    QVector<SearchResult> results;

    if (!m_db || !m_searchStmt) return results;

    const QString safeQuery = '"' + query.trimmed().replace('"', ' ') + '"';
    const QByteArray queryBytes = safeQuery.toUtf8();
    const QByteArray shaBytes   = bookSha.toUtf8();

    sqlite3_reset(m_searchStmt);
    sqlite3_clear_bindings(m_searchStmt);
    sqlite3_bind_text(m_searchStmt, 1, queryBytes.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(m_searchStmt, 2, shaBytes.constData(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (m_searchStmt, 3, limit);

    while (sqlite3_step(m_searchStmt) == SQLITE_ROW) {
        SearchResult r;
        r.pageNum = sqlite3_column_int(m_searchStmt, 0);
        if (const auto *raw = sqlite3_column_text(m_searchStmt, 1))
            r.chapterTitle = QString::fromUtf8(reinterpret_cast<const char *>(raw));
        if (const auto *raw = sqlite3_column_text(m_searchStmt, 2))
            r.snippet = QString::fromUtf8(reinterpret_cast<const char *>(raw));
        results.append(r);
    }

    return results;
}

bool SearchDatabase::execDDL(const char *sql)
{
    char *errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qCritical() << "SearchDatabase DDL error:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool SearchDatabase::createSchema()
{
    return execDDL(kCreateBookPages)
        && execDDL(kCreatePageFts)
        && execDDL(kCreateBookIndex);
}

bool SearchDatabase::prepareSearchStatement()
{
    if (!m_db) return false;

    if (sqlite3_prepare_v2(m_db, kSearchSql, -1, &m_searchStmt, nullptr) != SQLITE_OK) {
        qWarning() << "SearchDatabase: prepare search stmt failed:"
                   << sqlite3_errmsg(m_db);
        return false;
    }
    return true;
}

QString SearchDatabase::bookSha(const QString &bookPath)
{
    const QString absPath = QFileInfo(bookPath).absoluteFilePath();
    const QByteArray hash =
        QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Sha256)
            .toHex();
    return QString::fromLatin1(hash);
}
