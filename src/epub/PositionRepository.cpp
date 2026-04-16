#include "PositionRepository.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

PositionRepository::PositionRepository(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("ReadMarkable"), QStringLiteral("reader"))
{
    qDebug() << "PositionRepository: storage at" << m_settings.fileName();
}

void PositionRepository::savePosition(const QString &bookPath, const QString &xpointer)
{
    if (bookPath.isEmpty() || xpointer.isEmpty()) {
        qWarning() << "PositionRepository::savePosition: empty bookPath or xpointer, skipping";
        return;
    }

    const QString key = bookKey(bookPath);
    m_settings.beginGroup(key);
    m_settings.setValue(QStringLiteral("xpointer"),  xpointer);
    m_settings.setValue(QStringLiteral("timestamp"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_settings.endGroup();
    m_settings.sync();

    qDebug() << "PositionRepository: saved position for" << QFileInfo(bookPath).fileName()
             << "->" << xpointer;
}

QString PositionRepository::loadPosition(const QString &bookPath) const
{
    if (bookPath.isEmpty()) {
        return {};
    }

    const QString key = bookKey(bookPath);

    m_settings.beginGroup(key);
    const QString xpointer = m_settings.value(QStringLiteral("xpointer")).toString();
    m_settings.endGroup();

    if (!xpointer.isEmpty()) {
        qDebug() << "PositionRepository: loaded position for"
                 << QFileInfo(bookPath).fileName() << "->" << xpointer;
    }
    return xpointer;
}

void PositionRepository::clearPosition(const QString &bookPath)
{
    if (bookPath.isEmpty()) {
        return;
    }

    const QString key = bookKey(bookPath);
    m_settings.beginGroup(key);
    m_settings.remove(QString());
    m_settings.endGroup();
    m_settings.sync();

    qDebug() << "PositionRepository: cleared position for"
             << QFileInfo(bookPath).fileName();
}

bool PositionRepository::hasPosition(const QString &bookPath) const
{
    if (bookPath.isEmpty()) {
        return false;
    }

    const QString key = bookKey(bookPath);
    m_settings.beginGroup(key);
    const bool has = m_settings.contains(QStringLiteral("xpointer"));
    m_settings.endGroup();
    return has;
}

void PositionRepository::saveProgress(const QString &bookPath,
                                      const QString &xpointer,
                                      int            progressPercent)
{
    if (bookPath.isEmpty() || xpointer.isEmpty()) {
        qWarning() << "PositionRepository::saveProgress: empty bookPath or xpointer, skipping";
        return;
    }

    const int clampedPercent = qBound(0, progressPercent, 100);
    const QString key = bookKey(bookPath);

    m_settings.beginGroup(key);
    m_settings.setValue(QStringLiteral("xpointer"),       xpointer);
    m_settings.setValue(QStringLiteral("timestamp"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_settings.setValue(QStringLiteral("progressPercent"), clampedPercent);
    m_settings.endGroup();
    m_settings.sync();

    qDebug() << "PositionRepository: saved progress for"
             << QFileInfo(bookPath).fileName()
             << " - " << clampedPercent << "% @" << xpointer;
}

void PositionRepository::saveProgress(const QString &bookPath,
                                      const QString &xpointer,
                                      int            progressPercent,
                                      const QString &chapterName)
{
    if (bookPath.isEmpty() || xpointer.isEmpty()) {
        qWarning() << "PositionRepository::saveProgress(4-arg): empty bookPath or xpointer, skipping";
        return;
    }

    const int clampedPercent = qBound(0, progressPercent, 100);
    const QString key = bookKey(bookPath);

    m_settings.beginGroup(key);
    m_settings.setValue(QStringLiteral("xpointer"),       xpointer);
    m_settings.setValue(QStringLiteral("timestamp"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_settings.setValue(QStringLiteral("progressPercent"), clampedPercent);
    if (!chapterName.isEmpty()) {
        m_settings.setValue(QStringLiteral("chapter"), chapterName);
    }
    m_settings.endGroup();
    m_settings.sync();

    qDebug() << "PositionRepository: saved progress+chapter for"
             << QFileInfo(bookPath).fileName()
             << " - " << clampedPercent << "% @" << xpointer
             << "chapter:" << chapterName;
}

int PositionRepository::loadProgressPercent(const QString &bookPath) const
{
    if (bookPath.isEmpty()) {
        return 0;
    }

    const QString key = bookKey(bookPath);
    m_settings.beginGroup(key);
    const int percent = m_settings.value(QStringLiteral("progressPercent"), 0).toInt();
    m_settings.endGroup();

    return qBound(0, percent, 100);
}

QString PositionRepository::loadCurrentChapter(const QString &bookPath) const
{
    if (bookPath.isEmpty()) {
        return {};
    }

    const QString key = bookKey(bookPath);
    m_settings.beginGroup(key);
    const QString chapter = m_settings.value(QStringLiteral("chapter")).toString();
    m_settings.endGroup();

    return chapter;
}

QDateTime PositionRepository::loadLastReadTimestamp(const QString &bookPath) const
{
    if (bookPath.isEmpty()) {
        return {};
    }

    const QString key = bookKey(bookPath);
    m_settings.beginGroup(key);
    const QString tsString = m_settings.value(QStringLiteral("timestamp")).toString();
    m_settings.endGroup();

    if (tsString.isEmpty()) {
        return {};
    }

    const QDateTime dt = QDateTime::fromString(tsString, Qt::ISODate);
    if (!dt.isValid()) {
        qWarning() << "PositionRepository: invalid timestamp string for"
                   << QFileInfo(bookPath).fileName() << ":" << tsString;
    }
    return dt;
}

QString PositionRepository::bookKey(const QString &bookPath) const
{

    const QString absPath = QFileInfo(bookPath).absoluteFilePath();

    const QByteArray hash =
        QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Sha256)
            .toHex();

    return QStringLiteral("positions/") + QString::fromLatin1(hash);
}
