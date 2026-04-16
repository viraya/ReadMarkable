#pragma once

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>

#include "XochitlEntry.h"

class XochitlMetadataParser;

class XochitlLibrarySource : public QObject
{
    Q_OBJECT

public:

    explicit XochitlLibrarySource(const QString &xochitlDir, QObject *parent = nullptr);

    Q_INVOKABLE void startWatching();

    const QList<XochitlEntry> &currentEntries() const { return m_entries; }

signals:

    void entryAdded(const XochitlEntry &entry);

    void entryRemoved(const QString &uuid);

    void scanComplete();

private slots:

    void onDirectoryChanged(const QString &path);

    void doRescan();

private:

    void rebuildEpubDescendantCounts();

    QString                    m_xochitlDir;
    QFileSystemWatcher         m_watcher;
    QTimer                     m_debounceTimer;
    XochitlMetadataParser     *m_parser;
    QList<XochitlEntry>        m_entries;
    QHash<QString, QDateTime>  m_lastMtimes;

    QSet<QString>              m_liveUuids;
};
