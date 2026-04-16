#pragma once

#include <QImage>
#include <QMutex>

#include <list>
#include <optional>

class PageCache
{
public:
    explicit PageCache(int capacity = 5);

    std::optional<QImage> get(int pageNumber);

    void insert(int pageNumber, const QImage &image);

    void invalidateAll();

    void invalidateFrom(int page);

    bool contains(int pageNumber) const;

    int size() const;

private:
    struct Entry {
        int    page;
        QImage image;
    };

    std::list<Entry>   m_entries;
    mutable QMutex     m_mutex;
    int                m_capacity;
};
