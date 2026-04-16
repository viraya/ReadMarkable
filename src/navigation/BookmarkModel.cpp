#include "BookmarkModel.h"
#include "BookmarkRepository.h"

#include "lvdocview.h"
#include "lvtinydom.h"

#include <QDebug>

BookmarkModel::BookmarkModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BookmarkModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant BookmarkModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const BookmarkDisplayEntry &entry = m_entries[index.row()];
    switch (role) {
    case XPointerRole:
        return entry.xpointer;
    case PageRole:
        return entry.page;
    case PercentRole:
        return entry.percent;
    case TimestampRole:
        return entry.timestamp;
    default:
        return {};
    }
}

QHash<int, QByteArray> BookmarkModel::roleNames() const
{
    return {
        { XPointerRole,  "xpointer"  },
        { PageRole,      "page"      },
        { PercentRole,   "percent"   },
        { TimestampRole, "timestamp" },
    };
}

void BookmarkModel::refresh(LVDocView *docView,
                             BookmarkRepository *repo,
                             const QString &bookPath)
{
    Q_ASSERT(repo);

    beginResetModel();
    m_entries.clear();

    const QVector<BookmarkEntry> stored = repo->load(bookPath);

    for (const BookmarkEntry &bm : stored) {
        BookmarkDisplayEntry entry;
        entry.xpointer  = bm.xpointer;
        entry.percent   = bm.percent;
        entry.timestamp = bm.timestamp;

        entry.page = 0;
        if (docView) {
            ldomDocument *dom = docView->getDocument();
            if (dom) {
                const std::string utf8 = bm.xpointer.toStdString();
                const lString32 xpStr  = Utf8ToUnicode(utf8.c_str());
                const ldomXPointer xp  = dom->createXPointer(xpStr);
                if (!xp.isNull()) {
                    const int page0 = docView->getBookmarkPage(xp);
                    entry.page = page0 + 1;
                }
            }
        }

        m_entries.append(entry);
    }

    endResetModel();
    emit countChanged();

    qDebug() << "BookmarkModel::refresh: loaded" << m_entries.size() << "bookmarks";
}

void BookmarkModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

int BookmarkModel::count() const
{
    return m_entries.size();
}

QString BookmarkModel::xpointerAt(int index) const
{
    if (index < 0 || index >= m_entries.size())
        return {};
    return m_entries[index].xpointer;
}
