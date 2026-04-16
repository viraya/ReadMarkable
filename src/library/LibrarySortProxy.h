#pragma once

#include <QSortFilterProxyModel>

class LibrarySortProxy : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(int filterMode READ filterMode WRITE setFilterMode NOTIFY filterModeChanged)

public:
    enum SortMode {
        LastRead      = 0,
        RecentlyAdded = 1,
        TitleAZ       = 2,
        AuthorAZ      = 3
    };
    Q_ENUM(SortMode)

    enum FilterMode {
        AllEntries  = 0,
        FoldersOnly = 1,
        BooksOnly   = 2
    };
    Q_ENUM(FilterMode)

    explicit LibrarySortProxy(QObject *parent = nullptr);

    int sortMode() const;

    Q_INVOKABLE void setSortMode(int mode);

    int filterMode() const;

    Q_INVOKABLE void setFilterMode(int mode);

signals:
    void sortModeChanged();
    void filterModeChanged();

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void loadPersistedSortMode();

    SortMode   m_sortMode   = LastRead;
    FilterMode m_filterMode = AllEntries;
};
