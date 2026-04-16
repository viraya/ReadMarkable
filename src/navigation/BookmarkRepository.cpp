#include "BookmarkRepository.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

BookmarkRepository::BookmarkRepository(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("ReadMarkable"), QStringLiteral("reader"))
{
    qDebug() << "BookmarkRepository: storage at" << m_settings.fileName();
}

void BookmarkRepository::save(const QString &bookPath,
                               const QString &xpointer,
                               int percent)
{
    if (bookPath.isEmpty() || xpointer.isEmpty()) {
        qWarning() << "BookmarkRepository::save: empty bookPath or xpointer  -  skipping";
        return;
    }

    if (hasBookmarkAt(bookPath, xpointer)) {
        qDebug() << "BookmarkRepository::save: duplicate bookmark, skipping";
        return;
    }

    const QString groupKey = bookKey(bookPath);
    m_settings.beginGroup(groupKey);

    const QStringList existingKeys = m_settings.childGroups();
    int nextIndex = 0;
    for (const QString &k : existingKeys) {
        bool ok = false;
        const int idx = k.toInt(&ok);
        if (ok && idx >= nextIndex)
            nextIndex = idx + 1;
    }

    m_settings.endGroup();

    const QString entryKey = groupKey + QStringLiteral("/") +
                             QString::number(nextIndex);
    m_settings.beginGroup(entryKey);
    m_settings.setValue(QStringLiteral("xpointer"),  xpointer);
    m_settings.setValue(QStringLiteral("percent"),   percent);
    m_settings.setValue(QStringLiteral("timestamp"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_settings.endGroup();
    m_settings.sync();

    qDebug() << "BookmarkRepository: saved bookmark" << nextIndex
             << "for" << QFileInfo(bookPath).fileName()
             << "at" << xpointer;
}

void BookmarkRepository::remove(const QString &bookPath, const QString &xpointer)
{
    if (bookPath.isEmpty() || xpointer.isEmpty())
        return;

    const QString groupKey = bookKey(bookPath);
    m_settings.beginGroup(groupKey);
    const QStringList indices = m_settings.childGroups();
    m_settings.endGroup();

    for (const QString &idxStr : indices) {
        const QString entryKey = groupKey + QStringLiteral("/") + idxStr;
        m_settings.beginGroup(entryKey);
        const QString stored = m_settings.value(QStringLiteral("xpointer")).toString();
        m_settings.endGroup();

        if (stored == xpointer) {

            m_settings.beginGroup(entryKey);
            m_settings.remove(QString());
            m_settings.endGroup();
            m_settings.sync();
            qDebug() << "BookmarkRepository: removed bookmark" << idxStr
                     << "for" << QFileInfo(bookPath).fileName();
            return;
        }
    }

    qDebug() << "BookmarkRepository::remove: xpointer not found:" << xpointer;
}

QVector<BookmarkEntry> BookmarkRepository::load(const QString &bookPath) const
{
    if (bookPath.isEmpty())
        return {};

    const QString groupKey = bookKey(bookPath);
    m_settings.beginGroup(groupKey);
    const QStringList indices = m_settings.childGroups();
    m_settings.endGroup();

    QVector<BookmarkEntry> result;
    result.reserve(indices.size());

    for (const QString &idxStr : indices) {
        const QString entryKey = groupKey + QStringLiteral("/") + idxStr;
        m_settings.beginGroup(entryKey);
        const QString xp = m_settings.value(QStringLiteral("xpointer")).toString();
        const int pct    = m_settings.value(QStringLiteral("percent"),  0).toInt();
        const QString ts = m_settings.value(QStringLiteral("timestamp")).toString();
        m_settings.endGroup();

        if (xp.isEmpty())
            continue;

        BookmarkEntry entry;
        entry.xpointer  = xp;
        entry.percent   = pct;
        entry.timestamp = ts;
        result.append(entry);
    }

    return result;
}

bool BookmarkRepository::hasBookmarkAt(const QString &bookPath,
                                        const QString &xpointer) const
{
    if (bookPath.isEmpty() || xpointer.isEmpty())
        return false;

    const QVector<BookmarkEntry> entries = load(bookPath);
    for (const BookmarkEntry &e : entries) {
        if (e.xpointer == xpointer)
            return true;
    }
    return false;
}

QString BookmarkRepository::bookKey(const QString &bookPath) const
{
    const QString absPath = QFileInfo(bookPath).absoluteFilePath();
    const QByteArray hash =
        QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Sha256)
            .toHex();
    return QStringLiteral("bookmarks/") + QString::fromLatin1(hash);
}
