#include "XochitlLibrarySource.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QtGlobal>

#include "XochitlMetadataParser.h"

#define XOCH_RESCAN_DEBUG 0

XochitlLibrarySource::XochitlLibrarySource(const QString &xochitlDir, QObject *parent)
    : QObject(parent)
    , m_xochitlDir(xochitlDir)
    , m_parser(new XochitlMetadataParser(xochitlDir, this))
{

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(500);

    connect(&m_debounceTimer, &QTimer::timeout, this, &XochitlLibrarySource::doRescan);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &XochitlLibrarySource::onDirectoryChanged);
}

void XochitlLibrarySource::startWatching()
{
    if (!QFileInfo::exists(m_xochitlDir)) {

        qWarning() << "XochitlLibrarySource: xochitl dir not found, silent fallback:"
                   << m_xochitlDir;
        emit scanComplete();
        return;
    }

    if (!m_watcher.addPath(m_xochitlDir)) {
        qWarning() << "XochitlLibrarySource: QFileSystemWatcher::addPath failed for:"
                   << m_xochitlDir
                   << "(watcher may already be active, or OS limit reached)";
    }

    doRescan();
}

void XochitlLibrarySource::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)

    if (!m_watcher.directories().contains(m_xochitlDir)) {
        if (QFileInfo::exists(m_xochitlDir)) {
            if (!m_watcher.addPath(m_xochitlDir)) {
                qWarning() << "XochitlLibrarySource: failed to re-add watch path after"
                           << "directory change notification:"
                           << m_xochitlDir;
            }
        }

    }

    m_debounceTimer.start();
}

void XochitlLibrarySource::doRescan()
{

    QHash<QString, QDateTime> currentMtimes;

    const QFileInfoList metaFiles =
        QDir(m_xochitlDir).entryInfoList({QStringLiteral("*.metadata")}, QDir::Files);

    for (const QFileInfo &fi : metaFiles) {

        const QString fileName = fi.fileName();
        static constexpr int kSuffixLen = 9;
        if (fileName.length() <= kSuffixLen)
            continue;
        const QString uuid = fileName.left(fileName.length() - kSuffixLen);
        if (!uuid.isEmpty())
            currentMtimes.insert(uuid, fi.lastModified());
    }

    QSet<QString> skippedNew;

#if XOCH_RESCAN_DEBUG
    int dbgTotal = currentMtimes.size();
    int dbgNewOrChanged = 0, dbgSucceeded = 0, dbgSkippedNew = 0;
#endif

    for (auto it = currentMtimes.constBegin(); it != currentMtimes.constEnd(); ++it) {
        const QString   &uuid  = it.key();
        const QDateTime &mtime = it.value();

        const bool isNew     = !m_lastMtimes.contains(uuid);
        const bool isChanged = !isNew && (m_lastMtimes.value(uuid) != mtime);

        if (!isNew && !isChanged)
            continue;

#if XOCH_RESCAN_DEBUG
        ++dbgNewOrChanged;
#endif

        auto opt = m_parser->parseEntry(uuid);

        if (isNew && !opt.has_value()) {
            skippedNew.insert(uuid);
#if XOCH_RESCAN_DEBUG
            ++dbgSkippedNew;
            qDebug() << "XochitlLibrarySource: first-sight-nullopt (GAP-E1 retry):" << uuid;
#endif
            continue;
        }

        for (int i = m_entries.size() - 1; i >= 0; --i) {
            if (m_entries.at(i).uuid == uuid) {
                m_entries.removeAt(i);
                break;
            }
        }

        if (opt.has_value()) {

            m_entries.append(opt.value());

            m_liveUuids.insert(uuid);
            emit entryAdded(opt.value());
#if XOCH_RESCAN_DEBUG
            ++dbgSucceeded;
#endif
        } else {

            if (m_liveUuids.contains(uuid)) {
                m_liveUuids.remove(uuid);
                emit entryRemoved(uuid);
            }
        }
    }

    for (auto it = m_lastMtimes.constBegin(); it != m_lastMtimes.constEnd(); ++it) {
        const QString &uuid = it.key();
        if (currentMtimes.contains(uuid))
            continue;

        for (int i = m_entries.size() - 1; i >= 0; --i) {
            if (m_entries.at(i).uuid == uuid) {
                m_entries.removeAt(i);
                break;
            }
        }

        if (m_liveUuids.contains(uuid)) {
            m_liveUuids.remove(uuid);
            emit entryRemoved(uuid);
        }
    }

    for (const QString &u : skippedNew)
        currentMtimes.remove(u);
    m_lastMtimes = currentMtimes;

#if XOCH_RESCAN_DEBUG
    qDebug() << "XochitlLibrarySource::doRescan:"
             << "total=" << dbgTotal
             << "newOrChanged=" << dbgNewOrChanged
             << "succeeded=" << dbgSucceeded
             << "skippedNew(GAP-E1 retry)=" << dbgSkippedNew
             << "liveUuids=" << m_liveUuids.size();
#endif

    rebuildEpubDescendantCounts();

    emit scanComplete();
}

void XochitlLibrarySource::rebuildEpubDescendantCounts()
{

    for (XochitlEntry &e : m_entries) {
        e.epubDescendantCount = 0;
    }

    if (m_entries.isEmpty())
        return;

    QHash<QString, XochitlEntry *> entryMap;
    entryMap.reserve(m_entries.size());
    for (XochitlEntry &e : m_entries) {
        entryMap.insert(e.uuid, &e);
    }

    QHash<QString, QList<QString>> childrenMap;
    childrenMap.reserve(m_entries.size());
    for (const XochitlEntry &e : m_entries) {
        const QString key = entryMap.contains(e.parent) ? e.parent : QStringLiteral("");
        childrenMap[key].append(e.uuid);
    }

    static constexpr int kMaxDepth = 20;

    std::function<int(const QString &, QSet<QString> &, int)> countDescendants =
        [&](const QString &uuid, QSet<QString> &visited, int depth) -> int {
        if (depth > kMaxDepth) {
            qWarning() << "XochitlLibrarySource: depth limit reached at uuid" << uuid;
            return 0;
        }
        if (visited.contains(uuid)) {
            qWarning() << "XochitlLibrarySource: cycle detected at uuid" << uuid;
            return 0;
        }
        visited.insert(uuid);

        int count = 0;
        const auto &children = childrenMap.value(uuid);
        for (const QString &childUuid : children) {
            const XochitlEntry *child = entryMap.value(childUuid, nullptr);
            if (!child)
                continue;
            if (child->isEpub)
                ++count;
            if (child->type == QLatin1String("CollectionType"))
                count += countDescendants(childUuid, visited, depth + 1);
        }

        visited.remove(uuid);
        return count;
    };

    for (XochitlEntry &e : m_entries) {
        if (e.type != QLatin1String("CollectionType"))
            continue;
        QSet<QString> visited;
        e.epubDescendantCount = countDescendants(e.uuid, visited, 0);
    }
}
