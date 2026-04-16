#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>
#include <cstdint>

struct sqlite3;

struct MarginNoteEntry {
    int64_t    id               = 0;
    QString    bookSha;
    QString    paragraphXPointer;
    QByteArray strokeData;
    int64_t    createdAt        = 0;
};

class MarginNoteRepository
{
public:

    explicit MarginNoteRepository(const QString &dbPath);
    ~MarginNoteRepository();

    MarginNoteRepository(const MarginNoteRepository &) = delete;
    MarginNoteRepository &operator=(const MarginNoteRepository &) = delete;

    int64_t insert(const MarginNoteEntry &entry);

    bool updateStrokes(int64_t id, const QByteArray &strokeData);

    bool remove(int64_t id);

    QVector<MarginNoteEntry> loadForBook(const QString &bookPath) const;

    bool isOpen() const { return m_db != nullptr; }

private:
    bool execDDL(const char *sql);
    bool createSchema();

    static QString bookSha(const QString &bookPath);

    sqlite3 *m_db = nullptr;
};
