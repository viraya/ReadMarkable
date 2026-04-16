#include "PageCache.h"

#include <QMutexLocker>

PageCache::PageCache(int capacity)
    : m_capacity(capacity > 0 ? capacity : 1)
{
}

std::optional<QImage> PageCache::get(int pageNumber)
{
    QMutexLocker lock(&m_mutex);

    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->page == pageNumber) {

            if (it != m_entries.begin())
                m_entries.splice(m_entries.begin(), m_entries, it);
            return it->image;
        }
    }
    return std::nullopt;
}

void PageCache::insert(int pageNumber, const QImage &image)
{
    QMutexLocker lock(&m_mutex);

    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->page == pageNumber) {
            it->image = image;
            if (it != m_entries.begin())
                m_entries.splice(m_entries.begin(), m_entries, it);
            return;
        }
    }

    m_entries.push_front({pageNumber, image});

    while (static_cast<int>(m_entries.size()) > m_capacity)
        m_entries.pop_back();
}

void PageCache::invalidateAll()
{
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
}

void PageCache::invalidateFrom(int page)
{
    QMutexLocker lock(&m_mutex);

    auto it = m_entries.begin();
    while (it != m_entries.end()) {
        if (it->page >= page)
            it = m_entries.erase(it);
        else
            ++it;
    }
}

bool PageCache::contains(int pageNumber) const
{
    QMutexLocker lock(&m_mutex);
    for (const auto &entry : m_entries) {
        if (entry.page == pageNumber)
            return true;
    }
    return false;
}

int PageCache::size() const
{
    QMutexLocker lock(&m_mutex);
    return static_cast<int>(m_entries.size());
}
