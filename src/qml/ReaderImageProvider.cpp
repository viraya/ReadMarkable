#include "ReaderImageProvider.h"
#include "rendering/PageCache.h"

#include <QDebug>
#include <QImage>
#include <QPainter>

ReaderImageProvider::ReaderImageProvider(PageCache *cache)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_cache(cache)
{
    Q_ASSERT(cache);
}

QImage ReaderImageProvider::requestImage(const QString &id, QSize *size,
                                          const QSize &requestedSize)
{

    int pageNumber = -1;
    if (id.startsWith(QLatin1String("page/"))) {
        QString pageStr = id.mid(5);
        const int queryIdx = pageStr.indexOf(QLatin1Char('?'));
        if (queryIdx >= 0)
            pageStr = pageStr.left(queryIdx);
        bool ok = false;
        pageNumber = pageStr.toInt(&ok);
        if (!ok) {
            qWarning() << "ReaderImageProvider: invalid page id:" << id;
            pageNumber = -1;
        }
    } else {
        qWarning() << "ReaderImageProvider: unrecognised id format:" << id;
    }

    if (pageNumber > 0) {
        auto cached = m_cache->get(pageNumber);
        if (cached.has_value() && !cached->isNull()) {
            if (size) *size = cached->size();
            return *cached;
        }
    }

    qDebug() << "ReaderImageProvider: cache miss for id:" << id
             << " -  returning placeholder";

    QSize placeholderSize = (requestedSize.isValid() && !requestedSize.isEmpty())
                                ? requestedSize
                                : QSize(954, 1696);

    QImage placeholder(placeholderSize, QImage::Format_Grayscale8);
    placeholder.fill(255);

    if (size) *size = placeholderSize;
    return placeholder;
}
