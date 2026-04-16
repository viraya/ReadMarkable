#include "LibrarySortProxy.h"

#include <QDateTime>
#include <QSettings>
#include <QString>

#include "library/LibraryModel.h"

static constexpr char kSettingsOrg[]     = "ReadMarkable";
static constexpr char kSettingsApp[]     = "reader";
static constexpr char kSortModeKey[]     = "library/sortMode";
static constexpr int  kDefaultSortMode   = 0;

LibrarySortProxy::LibrarySortProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    loadPersistedSortMode();

    setSortRole(LibraryModel::TitleRole);
    setDynamicSortFilter(true);

    sort(0);
}

int LibrarySortProxy::sortMode() const
{
    return static_cast<int>(m_sortMode);
}

void LibrarySortProxy::setSortMode(int mode)
{
    const SortMode newMode = static_cast<SortMode>(mode);
    if (newMode == m_sortMode)
        return;

    m_sortMode = newMode;

    QSettings settings(kSettingsOrg, kSettingsApp);
    settings.setValue(kSortModeKey, static_cast<int>(m_sortMode));

    invalidate();

    emit sortModeChanged();
}

bool LibrarySortProxy::lessThan(const QModelIndex &left,
                                const QModelIndex &right) const
{
    const auto *src = sourceModel();

    const bool leftIsFolder  = src->data(left,  LibraryModel::IsFolderRole).toBool();
    const bool rightIsFolder = src->data(right, LibraryModel::IsFolderRole).toBool();

    if (leftIsFolder != rightIsFolder)
        return leftIsFolder;

    if (leftIsFolder && rightIsFolder) {
        const QString lTitle = src->data(left,  LibraryModel::TitleRole).toString();
        const QString rTitle = src->data(right, LibraryModel::TitleRole).toString();
        return QString::localeAwareCompare(lTitle.toLower(), rTitle.toLower()) < 0;
    }

    switch (m_sortMode) {

    case LastRead: {
        const QDateTime lDt = src->data(left,  LibraryModel::LastReadRole).toDateTime();
        const QDateTime rDt = src->data(right, LibraryModel::LastReadRole).toDateTime();

        if (!lDt.isValid() && !rDt.isValid()) {

            const QDateTime lAdded = src->data(left,  LibraryModel::AddedDateRole).toDateTime();
            const QDateTime rAdded = src->data(right, LibraryModel::AddedDateRole).toDateTime();
            if (!lAdded.isValid() && !rAdded.isValid()) return false;
            if (!lAdded.isValid()) return false;
            if (!rAdded.isValid()) return true;
            return lAdded > rAdded;
        }
        if (!lDt.isValid()) return false;
        if (!rDt.isValid()) return true;
        return lDt > rDt;
    }

    case RecentlyAdded: {
        const QDateTime lDt = src->data(left,  LibraryModel::AddedDateRole).toDateTime();
        const QDateTime rDt = src->data(right, LibraryModel::AddedDateRole).toDateTime();

        if (!lDt.isValid() && !rDt.isValid()) return false;
        if (!lDt.isValid()) return false;
        if (!rDt.isValid()) return true;
        return lDt > rDt;
    }

    case TitleAZ: {
        const QString lTitle = src->data(left,  LibraryModel::TitleRole).toString();
        const QString rTitle = src->data(right, LibraryModel::TitleRole).toString();

        return QString::localeAwareCompare(lTitle.toLower(), rTitle.toLower()) < 0;
    }

    case AuthorAZ: {
        const QString lAuthor = src->data(left,  LibraryModel::AuthorRole).toString();
        const QString rAuthor = src->data(right, LibraryModel::AuthorRole).toString();

        if (lAuthor.isEmpty() && rAuthor.isEmpty()) return false;
        if (lAuthor.isEmpty()) return false;
        if (rAuthor.isEmpty()) return true;

        return QString::localeAwareCompare(lAuthor.toLower(), rAuthor.toLower()) < 0;
    }

    }

    return false;
}

int LibrarySortProxy::filterMode() const
{
    return static_cast<int>(m_filterMode);
}

void LibrarySortProxy::setFilterMode(int mode)
{
    const FilterMode newMode = static_cast<FilterMode>(mode);
    if (newMode == m_filterMode)
        return;

    m_filterMode = newMode;
    invalidateFilter();
    emit filterModeChanged();
}

bool LibrarySortProxy::filterAcceptsRow(int sourceRow,
                                        const QModelIndex &sourceParent) const
{
    if (m_filterMode == AllEntries)
        return true;

    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    const bool isFolder = sourceModel()->data(idx, LibraryModel::IsFolderRole).toBool();

    return (m_filterMode == FoldersOnly) ? isFolder : !isFolder;
}

void LibrarySortProxy::loadPersistedSortMode()
{
    QSettings settings(kSettingsOrg, kSettingsApp);
    const int raw = settings.value(kSortModeKey, kDefaultSortMode).toInt();

    if (raw >= LastRead && raw <= AuthorAZ) {
        m_sortMode = static_cast<SortMode>(raw);
    } else {
        m_sortMode = LastRead;
    }
}
