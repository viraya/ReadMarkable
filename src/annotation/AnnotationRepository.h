#pragma once

#include "AnnotationTypes.h"

#include <QVector>
#include <QString>

struct sqlite3;

class AnnotationRepository
{
public:

    explicit AnnotationRepository(const QString &dbPath);

    ~AnnotationRepository();

    AnnotationRepository(const AnnotationRepository &) = delete;
    AnnotationRepository &operator=(const AnnotationRepository &) = delete;

    int64_t insert(const AnnotationEntry &entry);

    bool update(int64_t id, AnnotationStyle style, const QString &note);

    bool remove(int64_t id);

    QVector<AnnotationEntry> loadForBook(const QString &bookPath) const;

    bool hasAnnotationAt(const QString &bookPath,
                         const QString &startXPtr,
                         const QString &endXPtr) const;

    bool isOpen() const { return m_db != nullptr; }

private:

    bool execDDL(const char *sql);

    bool createSchema();

    static QString bookSha(const QString &bookPath);

    sqlite3 *m_db = nullptr;
};
