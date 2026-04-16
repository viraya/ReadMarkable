#pragma once

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>
#include <QString>

class QuaZip;

class CoverImageProvider : public QQuickImageProvider
{
public:
    CoverImageProvider();

    QImage requestImage(const QString &idIn, QSize *size,
                        const QSize &requestedSize) override;

private:

    static QString readContainerXml(QuaZip &zip);

    static QString readCoverHrefFromOpf(QuaZip &zip, const QString &opfPath);

    static QImage readCoverImage(QuaZip &zip, const QString &coverPath,
                                 const QSize &targetSize);

    void cacheInsert(const QString &epubPath, const QImage &image);

    QImage tryLoadExistingThumbnail(const QString &thumbPath,
                                    const QSize &targetSize);

    QImage tryLoadFromDiskCache(const QString &epubPath,
                                const QSize &targetSize);

    QImage tryExtractEpubCover(const QString &epubPath,
                               const QSize &targetSize);

    void saveToDiskCache(const QString &epubPath, const QImage &image);

    static QString diskCachePath(const QString &epubPath);

    QImage generatePlaceholder(const QString &title, const QSize &targetSize);

    QHash<QString, QImage> m_cache;
    QMutex m_mutex;

    static constexpr QSize DEFAULT_COVER_SIZE { 220, 310 };
    static constexpr int   MAX_CACHE_SIZE = 200;
};
