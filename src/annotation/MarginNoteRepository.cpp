#include "MarginNoteRepository.h"

#include <vendor/sqlite3/sqlite3.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

static const char *kCreateTable = R"sql(
CREATE TABLE IF NOT EXISTS margin_notes (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    book_sha         TEXT    NOT NULL,
    paragraph_xptr   TEXT    NOT NULL,
    stroke_data      BLOB    NOT NULL,
    created_at       INTEGER NOT NULL
);
)sql";

static const char *kCreateIndex = R"sql(
CREATE INDEX IF NOT EXISTS idx_mn_book ON margin_notes(book_sha);
)sql";

MarginNoteRepository::MarginNoteRepository(const QString &dbPath)
{
    const QByteArray pathBytes = dbPath.toUtf8();
    const int rc = sqlite3_open(pathBytes.constData(), &m_db);
    if (rc != SQLITE_OK) {
        qCritical() << "MarginNoteRepository: cannot open database"
                    << dbPath << ":" << sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    qDebug() << "MarginNoteRepository: opened" << dbPath;

    if (!createSchema()) {
        qCritical() << "MarginNoteRepository: schema creation failed for" << dbPath;
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

MarginNoteRepository::~MarginNoteRepository()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

int64_t MarginNoteRepository::insert(const MarginNoteEntry &entry)
{
    if (!m_db) return -1;

    const char *sql =
        "INSERT INTO margin_notes "
        "(book_sha, paragraph_xptr, stroke_data, created_at) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "MarginNoteRepository::insert prepare error:"
                   << sqlite3_errmsg(m_db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, entry.bookSha.toUtf8().constData(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, entry.paragraphXPointer.toUtf8().constData(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, entry.strokeData.constData(),
                      entry.strokeData.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, entry.createdAt);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        qWarning() << "MarginNoteRepository::insert step error:" << sqlite3_errmsg(m_db);
        return -1;
    }

    const int64_t rowid = sqlite3_last_insert_rowid(m_db);
    qDebug() << "MarginNoteRepository: inserted margin note id" << rowid;
    return rowid;
}

bool MarginNoteRepository::updateStrokes(int64_t id, const QByteArray &strokeData)
{
    if (!m_db) return false;

    const char *sql =
        "UPDATE margin_notes SET stroke_data = ? WHERE id = ?";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "MarginNoteRepository::updateStrokes prepare error:"
                   << sqlite3_errmsg(m_db);
        return false;
    }

    sqlite3_bind_blob (stmt, 1, strokeData.constData(),
                       strokeData.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        qWarning() << "MarginNoteRepository::updateStrokes step error:" << sqlite3_errmsg(m_db);
        return false;
    }

    qDebug() << "MarginNoteRepository: updated strokes for id" << id;
    return true;
}

bool MarginNoteRepository::remove(int64_t id)
{
    if (!m_db) return false;

    const char *sql = "DELETE FROM margin_notes WHERE id = ?";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "MarginNoteRepository::remove prepare error:"
                   << sqlite3_errmsg(m_db);
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        qWarning() << "MarginNoteRepository::remove step error:" << sqlite3_errmsg(m_db);
        return false;
    }

    qDebug() << "MarginNoteRepository: removed margin note id" << id;
    return true;
}

QVector<MarginNoteEntry> MarginNoteRepository::loadForBook(const QString &bookPath) const
{
    if (!m_db) return {};

    const QString sha = bookSha(bookPath);

    const char *sql =
        "SELECT id, book_sha, paragraph_xptr, stroke_data, created_at "
        "FROM margin_notes "
        "WHERE book_sha = ? "
        "ORDER BY created_at ASC";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "MarginNoteRepository::loadForBook prepare error:"
                   << sqlite3_errmsg(m_db);
        return {};
    }

    sqlite3_bind_text(stmt, 1, sha.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    QVector<MarginNoteEntry> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MarginNoteEntry e;
        e.id = sqlite3_column_int64(stmt, 0);
        e.bookSha = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        e.paragraphXPointer = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));

        const void *blobData  = sqlite3_column_blob(stmt, 3);
        const int   blobBytes = sqlite3_column_bytes(stmt, 3);
        if (blobData && blobBytes > 0) {
            e.strokeData = QByteArray(static_cast<const char *>(blobData), blobBytes);
        }

        e.createdAt = sqlite3_column_int64(stmt, 4);
        result.append(e);
    }

    sqlite3_finalize(stmt);
    qDebug() << "MarginNoteRepository: loaded" << result.size()
             << "margin notes for" << QFileInfo(bookPath).fileName();
    return result;
}

bool MarginNoteRepository::execDDL(const char *sql)
{
    char *errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qCritical() << "MarginNoteRepository DDL error:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool MarginNoteRepository::createSchema()
{
    return execDDL(kCreateTable) && execDDL(kCreateIndex);
}

QString MarginNoteRepository::bookSha(const QString &bookPath)
{
    const QString absPath = QFileInfo(bookPath).absoluteFilePath();
    const QByteArray hash =
        QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Sha256)
            .toHex();
    return QString::fromLatin1(hash);
}
