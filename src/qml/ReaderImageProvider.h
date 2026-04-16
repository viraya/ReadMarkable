#pragma once

#include <QQuickImageProvider>

class PageCache;

class ReaderImageProvider : public QQuickImageProvider
{
public:
    explicit ReaderImageProvider(PageCache *cache);

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

private:
    PageCache *m_cache;
};
