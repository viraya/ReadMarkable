#include "TocModel.h"

#include "lvdocview.h"
#include "lvtinydom.h"

#include <QDebug>

TocModel::TocModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TocModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant TocModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const TocEntry &entry = m_entries[index.row()];
    switch (role) {
    case TitleRole:
        return entry.title;
    case LevelRole:
        return entry.level;
    case XPointerRole:
        return entry.xpointer;
    case PageRole:
        return entry.pageDisplay;
    default:
        return {};
    }
}

QHash<int, QByteArray> TocModel::roleNames() const
{
    return {
        { TitleRole,    "title"    },
        { LevelRole,    "level"    },
        { XPointerRole, "xpointer" },
        { PageRole,     "page"     },
    };
}

void TocModel::populate(LVDocView *docView)
{
    Q_ASSERT(docView);

    beginResetModel();
    m_entries.clear();

    LVPtrVector<LVTocItem, false> items;
    const bool hasToc = docView->getFlatToc(items);

    if (!hasToc || items.length() == 0) {
        qDebug() << "TocModel::populate: document has no TOC";
        endResetModel();
        emit countChanged();
        return;
    }

    for (int i = 0; i < items.length(); ++i) {
        LVTocItem *item = items[i];
        if (!item)
            continue;

        const int crLevel = item->getLevel();

        if (crLevel == 0 || crLevel > 3)
            continue;

        TocEntry entry;

        entry.level = crLevel - 1;

        entry.page0       = item->getPage();
        entry.pageDisplay = entry.page0 + 1;

        const lString32 name = item->getName();
        entry.title = QString::fromUtf8(UnicodeToUtf8(name).c_str());

        ldomXPointer xp = item->getXPointer();
        if (!xp.isNull()) {
            const lString32 xpStr = xp.toString();
            entry.xpointer = QString::fromUtf8(UnicodeToUtf8(xpStr).c_str());
        }

        m_entries.append(entry);
    }

    endResetModel();
    emit countChanged();

    qDebug() << "TocModel::populate: loaded" << m_entries.size() << "TOC entries";
}

void TocModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

int TocModel::count() const
{
    return m_entries.size();
}

QString TocModel::xpointerAt(int index) const
{
    if (index < 0 || index >= m_entries.size())
        return {};
    return m_entries[index].xpointer;
}

int TocModel::findChapterForPage(int page0based) const
{

    int result = -1;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].page0 <= page0based)
            result = i;
        else
            break;
    }
    return result;
}
