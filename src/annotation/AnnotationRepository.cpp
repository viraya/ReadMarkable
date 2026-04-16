#include "AnnotationRepository.h"

#include <vendor/sqlite3/sqlite3.h>

#include <QCryptographicHash>
#include <QDebug>
#include <QFileInfo>

static const char *kCreateTable = R"sql(
CREATE TABLE IF NOT EXISTS annotations (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    book_sha      TEXT    NOT NULL,
    start_xptr    TEXT    NOT NULL,
    end_xptr      TEXT    NOT NULL,
    selected_text TEXT    NOT NULL,
    style         INTEGER NOT NULL DEFAULT 0,
    note          TEXT    DEFAULT '',
    created_at    INTEGER NOT NULL,
    chapter_title TEXT    DEFAULT ''
);
)sql";

static const char *kCreateIndex = R"sql(
CREATE INDEX IF NOT EXISTS idx_ann_book ON annotations(book_sha);
)sql";

AnnotationRepository::AnnotationRepository(const QString &dbPath)
{
    const QByteArray pathBytes = dbPath.toUtf8();
    const int rc = sqlite3_open(pathBytes.constData(), &m_db);
    if (rc != SQLITE_OK) {
        qCritical() << "AnnotationRepository: cannot open database"
                    << dbPath << ":" << sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }

    qDebug() << "AnnotationRepository: opened" << dbPath;

    if (!createSchema()) {
        qCritical() << "AnnotationRepository: schema creation failed for" << dbPath;
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

AnnotationRepository::~AnnotationRepository()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

int64_t AnnotationRepository::insert(const AnnotationEntry &entry)
{
    if (!m_db) return -1;

    const char *sql =
        "INSERT INTO annotations "
        "(book_sha, start_xptr, end_xptr, selected_text, style, note, created_at, chapter_title) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "AnnotationRepository::insert prepare error:"
                   << sqlite3_errmsg(m_db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, entry.bookSha.toUtf8().constData(),       -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, entry.startXPointer.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, entry.endXPointer.toUtf8().constData(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, entry.selectedText.toUtf8().constData(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 5, static_cast<int>(entry.style));
    sqlite3_bind_text(stmt, 6, entry.note.toUtf8().constData(),          -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, entry.createdAt);
    sqlite3_bind_text(stmt, 8, entry.chapterTitle.toUtf8().constData(),  -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        qWarning() << "AnnotationRepository::insert step error:" << sqlite3_errmsg(m_db);
        return -1;
    }

    const int64_t rowid = sqlite3_last_insert_rowid(m_db);
    qDebug() << "AnnotationRepository: inserted annotation id" << rowid;
    return rowid;
}

bool AnnotationRepository::update(int64_t id, AnnotationStyle style, const QString &note)
{
    if (!m_db) return false;

    const char *sql =
        "UPDATE annotations SET style = ?, note = ? WHERE id = ?";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "AnnotationRepository::update prepare error:"
                   << sqlite3_errmsg(m_db);
        return false;
    }

    sqlite3_bind_int  (stmt, 1, static_cast<int>(style));
    sqlite3_bind_text (stmt, 2, note.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, id);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        qWarning() << "AnnotationRepository::update step error:" << sqlite3_errmsg(m_db);
        return false;
    }

    qDebug() << "AnnotationRepository: updated annotation id" << id;
    return true;
}

bool AnnotationRepository::remove(int64_t id)
{
    if (!m_db) return false;

    const char *sql = "DELETE FROM annotations WHERE id = ?";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "AnnotationRepository::remove prepare error:"
                   << sqlite3_errmsg(m_db);
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        qWarning() << "AnnotationRepository::remove step error:" << sqlite3_errmsg(m_db);
        return false;
    }

    qDebug() << "AnnotationRepository: removed annotation id" << id;
    return true;
}

QVector<AnnotationEntry> AnnotationRepository::loadForBook(const QString &bookPath) const
{
    if (!m_db) return {};

    const QString sha = bookSha(bookPath);

    const char *sql =
        "SELECT id, book_sha, start_xptr, end_xptr, selected_text, "
        "       style, note, created_at, chapter_title "
        "FROM annotations "
        "WHERE book_sha = ? "
        "ORDER BY created_at ASC";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "AnnotationRepository::loadForBook prepare error:"
                   << sqlite3_errmsg(m_db);
        return {};
    }

    sqlite3_bind_text(stmt, 1, sha.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    QVector<AnnotationEntry> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AnnotationEntry e;
        e.id           = sqlite3_column_int64(stmt, 0);
        e.bookSha      = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        e.startXPointer = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        e.endXPointer  = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        e.selectedText = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)));
        e.style        = static_cast<AnnotationStyle>(sqlite3_column_int(stmt, 5));
        e.note         = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6)));
        e.createdAt    = sqlite3_column_int64(stmt, 7);
        e.chapterTitle = QString::fromUtf8(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 8)));
        result.append(e);
    }

    sqlite3_finalize(stmt);
    qDebug() << "AnnotationRepository: loaded" << result.size()
             << "annotations for" << QFileInfo(bookPath).fileName();
    return result;
}

bool AnnotationRepository::hasAnnotationAt(const QString &bookPath,
                                            const QString &startXPtr,
                                            const QString &endXPtr) const
{
    if (!m_db) return false;

    const QString sha = bookSha(bookPath);

    const char *sql =
        "SELECT 1 FROM annotations "
        "WHERE book_sha = ? AND start_xptr = ? AND end_xptr = ? "
        "LIMIT 1";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "AnnotationRepository::hasAnnotationAt prepare error:"
                   << sqlite3_errmsg(m_db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, sha.toUtf8().constData(),          -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, startXPtr.toUtf8().constData(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, endXPtr.toUtf8().constData(),      -1, SQLITE_TRANSIENT);

    const bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

bool AnnotationRepository::execDDL(const char *sql)
{
    char *errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qCritical() << "AnnotationRepository DDL error:" << errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool AnnotationRepository::createSchema()
{
    return execDDL(kCreateTable) && execDDL(kCreateIndex);
}

QString AnnotationRepository::bookSha(const QString &bookPath)
{
    const QString absPath = QFileInfo(bookPath).absoluteFilePath();
    const QByteArray hash =
        QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Sha256)
            .toHex();
    return QString::fromLatin1(hash);
}
