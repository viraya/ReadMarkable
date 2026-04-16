#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVector>

struct BookmarkEntry {
    QString xpointer;
    int     percent  = 0;
    QString timestamp;
};

class BookmarkRepository : public QObject
{
    Q_OBJECT

public:
    explicit BookmarkRepository(QObject *parent = nullptr);

    void save(const QString &bookPath, const QString &xpointer, int percent);

    void remove(const QString &bookPath, const QString &xpointer);

    QVector<BookmarkEntry> load(const QString &bookPath) const;

    bool hasBookmarkAt(const QString &bookPath, const QString &xpointer) const;

private:

    QString bookKey(const QString &bookPath) const;

    mutable QSettings m_settings;
};
