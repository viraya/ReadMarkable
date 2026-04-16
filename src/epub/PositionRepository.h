#pragma once

#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QString>

class PositionRepository : public QObject
{
    Q_OBJECT

public:
    explicit PositionRepository(QObject *parent = nullptr);

    void savePosition(const QString &bookPath, const QString &xpointer);

    QString loadPosition(const QString &bookPath) const;

    void clearPosition(const QString &bookPath);

    bool hasPosition(const QString &bookPath) const;

    void saveProgress(const QString &bookPath, const QString &xpointer, int progressPercent);

    void saveProgress(const QString &bookPath, const QString &xpointer,
                      int progressPercent, const QString &chapterName);

    int loadProgressPercent(const QString &bookPath) const;

    QString loadCurrentChapter(const QString &bookPath) const;

    QDateTime loadLastReadTimestamp(const QString &bookPath) const;

private:

    QString bookKey(const QString &bookPath) const;

    mutable QSettings m_settings;
};
