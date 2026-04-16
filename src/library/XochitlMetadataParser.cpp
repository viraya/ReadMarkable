#include "XochitlMetadataParser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QtGlobal>
#include <optional>

static constexpr int kMaxRetries   = 3;
static constexpr int kRetryDelayMs = 10;
static constexpr int kMaxDepth     = 20;

static constexpr Qt::DateFormat kDateTimeFormat = Qt::ISODateWithMs;

XochitlMetadataParser::XochitlMetadataParser(QString xochitlDir, QObject *parent)
    : QObject(parent)
    , m_xochitlDir(std::move(xochitlDir))
{
}

QList<XochitlEntry> XochitlMetadataParser::scanAll()
{
    QDir dir(m_xochitlDir);
    if (!dir.exists()) {

        return {};
    }

    const QStringList metaFiles = dir.entryList({QStringLiteral("*.metadata")}, QDir::Files);

    QList<XochitlEntry> entries;
    entries.reserve(metaFiles.size());

    for (const QString &fileName : metaFiles) {

        const QString uuid = fileName.left(fileName.length() - qstrlen(".metadata"));
        if (uuid.isEmpty()) {
            continue;
        }

        auto opt = parseEntry(uuid);
        if (opt.has_value()) {
            entries.append(std::move(opt.value()));
        }
    }

    populateEpubDescendantCounts(entries);

    return entries;
}

std::optional<XochitlEntry> XochitlMetadataParser::parseEntry(const QString &uuid)
{

    const QString base           = m_xochitlDir + QLatin1Char('/') + uuid;
    const QString metaPath       = base + QStringLiteral(".metadata");
    const QString contentPath    = base + QStringLiteral(".content");
    const QString epubPath       = base + QStringLiteral(".epub");
    const QString tombstonePath  = base + QStringLiteral(".tombstone");
    const QString thumbnailPath  = base + QStringLiteral(".thumbnails/0.jpg");

    if (QFileInfo::exists(tombstonePath)) {
        return std::nullopt;
    }

    const auto metaOpt = loadJsonWithRetry(metaPath);
    if (!metaOpt.has_value()) {

        return std::nullopt;
    }
    const QJsonObject &meta = metaOpt.value();

    XochitlEntry entry;
    entry.uuid        = uuid;
    entry.visibleName = meta[QStringLiteral("visibleName")].toString(uuid);
    entry.type        = meta[QStringLiteral("type")].toString();
    entry.parent      = meta[QStringLiteral("parent")].toString();
    entry.deleted     = meta[QStringLiteral("deleted")].toBool(false);

    entry.lastOpened   = QDateTime::fromString(
        meta[QStringLiteral("lastOpened")].toString(), kDateTimeFormat);
    entry.lastModified = QDateTime::fromString(
        meta[QStringLiteral("lastModified")].toString(), kDateTimeFormat);

    if (entry.deleted || entry.parent == QLatin1String("trash")) {
        return std::nullopt;
    }

    if (entry.type == QLatin1String("DocumentType")) {

        QJsonObject contentObj;
        const auto contentOpt = loadJsonWithRetry(contentPath);
        if (contentOpt.has_value()) {
            contentObj = contentOpt.value();
        }

        entry.fileType = contentObj[QStringLiteral("fileType")].toString();

        {
            const QJsonObject docMeta =
                contentObj[QStringLiteral("documentMetadata")].toObject();
            if (!docMeta.isEmpty()) {
                const QJsonValue av = docMeta[QStringLiteral("authors")];
                if (av.isArray()) {
                    const QJsonArray arr = av.toArray();
                    QStringList names;
                    for (const QJsonValue &v : arr) {
                        const QString s = v.toString().trimmed();
                        if (!s.isEmpty()) {
                            names.append(s);
                        }
                    }
                    entry.author = names.join(QStringLiteral(", "));
                } else if (av.isString()) {
                    entry.author = av.toString().trimmed();
                }
            }
        }

        const bool isFileTypeEpub  = (entry.fileType == QLatin1String("epub"));
        const bool isFileTypeMissing = entry.fileType.isEmpty();
        const bool epubFileExists  = QFileInfo::exists(epubPath);

        if (isFileTypeEpub || (isFileTypeMissing && epubFileExists)) {
            entry.isEpub      = true;
            entry.epubFilePath = epubPath;
        } else {
            entry.isEpub      = false;
            entry.epubFilePath.clear();
        }

        if (QFileInfo::exists(thumbnailPath)) {
            entry.thumbnailPath = thumbnailPath;
        }

    } else if (entry.type == QLatin1String("CollectionType")) {

        entry.isEpub      = false;
        entry.epubFilePath.clear();

    }

    return entry;
}

std::optional<QJsonObject> XochitlMetadataParser::loadJsonWithRetry(const QString &path) const
{
    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {

            return std::nullopt;
        }

        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();

        if (!doc.isNull() && doc.isObject()) {
            return doc.object();
        }

        if (attempt == kMaxRetries - 1) {
            qWarning() << "XochitlMetadataParser: parse failed after"
                       << kMaxRetries << "retries:" << path
                       << "-" << err.errorString();
        } else {

            QThread::msleep(kRetryDelayMs);
        }
    }

    return std::nullopt;
}

namespace {

int countEpubDescendants(
    const QString                           &uuid,
    const QHash<QString, QList<QString>>    &childrenMap,
    const QHash<QString, XochitlEntry *>    &entryMap,
    QSet<QString>                           &visited,
    int                                      depth)
{
    if (depth > kMaxDepth) {
        qWarning() << "XochitlMetadataParser: depth limit reached at uuid" << uuid
                   << " -  possible deep nesting or unflushed cycle";
        return 0;
    }

    if (visited.contains(uuid)) {
        qWarning() << "XochitlMetadataParser: cycle detected at uuid" << uuid
                   << " -  skipping subtree";
        return 0;
    }

    visited.insert(uuid);

    int count = 0;
    const auto &children = childrenMap.value(uuid);
    for (const QString &childUuid : children) {
        const XochitlEntry *child = entryMap.value(childUuid, nullptr);
        if (!child) {
            continue;
        }
        if (child->isEpub) {
            ++count;
        }
        if (child->type == QLatin1String("CollectionType")) {
            count += countEpubDescendants(childUuid, childrenMap, entryMap, visited, depth + 1);
        }
    }

    visited.remove(uuid);
    return count;
}

}

void XochitlMetadataParser::populateEpubDescendantCounts(QList<XochitlEntry> &entries) const
{
    if (entries.isEmpty()) {
        return;
    }

    QHash<QString, XochitlEntry *> entryMap;
    entryMap.reserve(entries.size());
    for (XochitlEntry &e : entries) {
        entryMap.insert(e.uuid, &e);
    }

    QHash<QString, QList<QString>> childrenMap;
    childrenMap.reserve(entries.size());

    for (const XochitlEntry &e : entries) {
        const QString &parentUuid = e.parent;

        const QString key = entryMap.contains(parentUuid) ? parentUuid : QStringLiteral("");
        childrenMap[key].append(e.uuid);
    }

    for (XochitlEntry &e : entries) {
        if (e.type != QLatin1String("CollectionType")) {
            continue;
        }
        QSet<QString> visited;
        e.epubDescendantCount = countEpubDescendants(
            e.uuid, childrenMap, entryMap, visited, 0);
    }
}
