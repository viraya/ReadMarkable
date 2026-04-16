#include "LibraryModel.h"
#include "XochitlLibrarySource.h"
#include "epub/PositionRepository.h"

#include <QDebug>
#include <QFileInfo>
#include <algorithm>

LibraryModel::LibraryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LibraryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_books.size();
}

QVariant LibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_books.size()) {
        return {};
    }

    const BookRecord &book = m_books.at(index.row());

    switch (static_cast<BookRole>(role)) {
    case FilePathRole:       return book.filePath;
    case TitleRole:          return book.title;
    case AuthorRole:         return book.author;
    case CoverKeyRole:

        return book.coverImagePath.isEmpty() ? book.filePath : book.coverImagePath;
    case ProgressPercentRole: return book.progressPercent;
    case LastReadRole:        return book.lastRead;
    case AddedDateRole:       return book.addedDate;
    case CurrentChapterRole:  return book.currentChapter;
    case IsFolderRole:        return book.isFolder;
    case IsEmptyRole:         return book.isEmpty;
    case FolderPathRole:      return book.folderPath;
    case BookCountRole:       return book.bookCount;
    case DescriptionRole:     return book.description;
    case XochitlUuidRole:     return book.xochitlUuid;
    }
    return {};
}

QHash<int, QByteArray> LibraryModel::roleNames() const
{
    return {
        { FilePathRole,        "filePath"        },
        { TitleRole,           "title"           },
        { AuthorRole,          "author"          },
        { CoverKeyRole,        "coverKey"        },
        { ProgressPercentRole, "progressPercent" },
        { LastReadRole,        "lastRead"        },
        { AddedDateRole,       "addedDate"       },
        { CurrentChapterRole,  "currentChapter"  },
        { IsFolderRole,        "isFolder"        },
        { IsEmptyRole,         "isEmpty"         },
        { FolderPathRole,      "folderPath"      },
        { BookCountRole,       "bookCount"       },
        { DescriptionRole,     "description"     },
        { XochitlUuidRole,     "xochitlUuid"     },
    };
}

void LibraryModel::setXochitlSource(XochitlLibrarySource *src)
{
    m_xochitl = src;
}

void LibraryModel::setCurrentlyOpenBookPath(const QString &path)
{
    m_currentlyOpenBookPath = path;
}

void LibraryModel::setCurrentFolder(const QString &folderKey)
{
    if (m_currentFolderKey == folderKey) {
        return;
    }
    m_currentFolderKey = folderKey;

    if (!folderKey.isEmpty()) {

        bool isXochitlFolder = false;
        for (const BookRecord &rec : std::as_const(m_allEntries)) {
            if (rec.isFolder && rec.source == BookSource::Xochitl
                    && rec.xochitlUuid == folderKey) {
                isXochitlFolder = true;
                break;
            }
        }

        if (!isXochitlFolder) {

            qDebug() << "LibraryModel::setCurrentFolder: filesystem path" << folderKey;
            emit rescanDirectoryRequested(folderKey);
            return;
        }
    }

    rebuildVisibleEntries();
}

void LibraryModel::addBook(const BookRecord &record)
{

    BookRecord stamped = record;
    stamped.source     = BookSource::CustomPath;

    mergeEntry(stamped);
}

void LibraryModel::onScanComplete()
{
    m_customScanDone = true;
    updateContinueReading();

    qDebug() << "LibraryModel: custom-path scan complete  - "
             << m_allEntries.size() << "total entries";

    emit scanFinished();
}

void LibraryModel::clear()
{
    if (m_books.isEmpty() && m_allEntries.isEmpty()) {
        return;
    }
    beginResetModel();
    m_books.clear();
    m_allEntries.clear();
    m_customScanDone  = false;
    m_xochitlScanDone = false;
    m_lastReadBookPath.clear();
    m_lastReadBookTitle.clear();
    m_lastReadBookProgress = 0;
    m_lastReadBookCurrentChapter.clear();
    endResetModel();
    qDebug() << "LibraryModel: cleared";
}

int LibraryModel::count() const
{
    return m_books.size();
}

void LibraryModel::rescan()
{

    beginResetModel();
    m_allEntries.erase(
        std::remove_if(m_allEntries.begin(), m_allEntries.end(),
                       [](const BookRecord &r) { return r.source == BookSource::CustomPath; }),
        m_allEntries.end());
    m_books.clear();
    m_customScanDone = false;
    endResetModel();

    emit rescanRequested();
}

void LibraryModel::refreshPositions()
{
    if (!m_positionRepo) {
        qWarning() << "LibraryModel::refreshPositions  -  no PositionRepository set; skipping";
        return;
    }

    int touched = 0;
    for (BookRecord &rec : m_allEntries) {
        if (rec.isFolder || rec.filePath.isEmpty()) continue;

        const int       newProgress = m_positionRepo->loadProgressPercent(rec.filePath);
        const QDateTime newLastRead = m_positionRepo->loadLastReadTimestamp(rec.filePath);
        const QString   newChapter  = m_positionRepo->loadCurrentChapter(rec.filePath);

        const bool changed = (rec.progressPercent != newProgress)
                          || (rec.lastRead         != newLastRead)
                          || (rec.currentChapter   != newChapter);
        if (!changed) continue;

        qDebug() << "LibraryModel::refreshPositions  - "
                 << QFileInfo(rec.filePath).fileName()
                 << "progress" << rec.progressPercent << "->" << newProgress
                 << "lastRead" << rec.lastRead.toString(Qt::ISODate)
                 << "->" << newLastRead.toString(Qt::ISODate)
                 << "chapter \"" << rec.currentChapter << "\" -> \"" << newChapter << "\"";

        rec.progressPercent = newProgress;
        rec.lastRead        = newLastRead;
        rec.currentChapter  = newChapter;

        for (int i = 0; i < m_books.size(); ++i) {
            if (m_books[i].filePath == rec.filePath) {
                m_books[i].progressPercent = newProgress;
                m_books[i].lastRead        = newLastRead;
                m_books[i].currentChapter  = newChapter;
                const QModelIndex idx = index(i, 0);
                emit dataChanged(idx, idx, {
                    ProgressPercentRole,
                    LastReadRole,
                    CurrentChapterRole
                });
                break;
            }
        }
        ++touched;
    }

    qDebug() << "LibraryModel::refreshPositions  -  updated" << touched
             << "book records;" << m_allEntries.size() << "scanned";

    updateContinueReading();
    qDebug() << "LibraryModel::refreshPositions  -  after updateContinueReading: lastReadBookPath=\""
             << m_lastReadBookPath << "\" title=\"" << m_lastReadBookTitle
             << "\" progress=" << m_lastReadBookProgress;
    emit scanFinished();
}

void LibraryModel::rescanDirectory(const QString &path)
{

    emit rescanDirectoryRequested(path);
}

void LibraryModel::onXochitlEntryAdded(const XochitlEntry &entry)
{

    if (entry.type == QStringLiteral("DocumentType") && !entry.isEpub) {
        return;
    }

    const BookRecord rec = xochitlEntryToBookRecord(entry);
    if (rec.filePath.isEmpty() && !rec.isFolder) {

        return;
    }

    mergeEntry(rec);

    qDebug() << "LibraryModel: xochitl entry added"
             << (rec.isFolder ? QStringLiteral("[folder]") : QStringLiteral("[book]"))
             << rec.title;
}

void LibraryModel::onXochitlEntryRemoved(const QString &uuid)
{

    QString removedFilePath;
    for (const BookRecord &rec : std::as_const(m_allEntries)) {
        if (rec.xochitlUuid == uuid) {
            removedFilePath = rec.filePath;
            break;
        }
    }

    removeXochitlEntry(uuid);

    if (!removedFilePath.isEmpty()
            && !m_currentlyOpenBookPath.isEmpty()
            && removedFilePath == m_currentlyOpenBookPath) {
        qDebug() << "LibraryModel: currently-open book externally deleted:" << removedFilePath;
        emit currentBookExternallyDeleted(removedFilePath);
    }
}

void LibraryModel::onXochitlScanComplete()
{
    m_xochitlScanDone = true;
    updateContinueReading();

    qDebug() << "LibraryModel: xochitl scan complete  - "
             << m_allEntries.size() << "total entries";

    emit scanFinished();
}

BookRecord LibraryModel::xochitlEntryToBookRecord(const XochitlEntry &entry) const
{
    BookRecord rec;
    rec.source          = BookSource::Xochitl;
    rec.xochitlUuid     = entry.uuid;

    rec.xochitlParentUuid = (entry.parent == QStringLiteral("trash")) ? QString() : entry.parent;
    rec.title           = entry.visibleName.isEmpty() ? entry.uuid : entry.visibleName;
    rec.author          = entry.author;
    rec.addedDate       = entry.lastModified;

    if (entry.type == QStringLiteral("CollectionType")) {

        rec.isFolder        = true;
        rec.folderPath      = QString();
        rec.bookCount       = entry.epubDescendantCount;
        rec.isEmpty         = (entry.epubDescendantCount == 0);

        qDebug() << "LibraryModel: xochitl folder" << rec.title
                 << "epubDescendantCount=" << entry.epubDescendantCount
                 << "bookCount=" << rec.bookCount
                 << "isEmpty=" << rec.isEmpty;
    } else {

        rec.isFolder        = false;
        rec.filePath        = entry.epubFilePath;
        rec.coverImagePath  = entry.thumbnailPath;
        rec.lastRead        = entry.lastOpened;

        if (m_positionRepo && !rec.filePath.isEmpty()) {
            rec.progressPercent = m_positionRepo->loadProgressPercent(rec.filePath);
            const QDateTime ts = m_positionRepo->loadLastReadTimestamp(rec.filePath);
            if (ts.isValid()) {
                rec.lastRead = ts;
            }
            const QString ch = m_positionRepo->loadCurrentChapter(rec.filePath);
            if (!ch.isEmpty()) {
                rec.currentChapter = ch;
            }
        }
    }

    return rec;
}

void LibraryModel::mergeEntry(const BookRecord &incoming)
{

    if (!incoming.isFolder && !incoming.filePath.isEmpty()) {
        for (int i = 0; i < m_allEntries.size(); ++i) {
            BookRecord &existing = m_allEntries[i];
            if (!existing.isFolder && existing.filePath == incoming.filePath) {

                if (incoming.source == BookSource::Xochitl
                        && existing.source == BookSource::CustomPath) {

                    qDebug() << "LibraryModel: upgrading" << existing.filePath
                             << "from CustomPath to Xochitl source";
                    existing = incoming;

                    rebuildVisibleEntries();
                }

                return;
            }
        }
    }

    if (incoming.isFolder && incoming.source == BookSource::Xochitl
            && !incoming.xochitlUuid.isEmpty()) {
        for (int i = 0; i < m_allEntries.size(); ++i) {
            BookRecord &existing = m_allEntries[i];
            if (existing.isFolder && existing.xochitlUuid == incoming.xochitlUuid) {

                existing = incoming;
                rebuildVisibleEntries();
                return;
            }
        }
    }

    m_allEntries.append(incoming);

    qDebug() << "LibraryModel: mergeEntry [new]"
             << (incoming.isFolder ? QStringLiteral("[folder]") : QStringLiteral("[book]"))
             << "src=" << static_cast<int>(incoming.source)
             << incoming.title;

    bool inCurrentFolder = false;
    const QString &fk = m_currentFolderKey;

    if (fk.isEmpty()) {

        if (incoming.source == BookSource::CustomPath) {
            inCurrentFolder = true;
        } else if (incoming.source == BookSource::Xochitl
                   && incoming.xochitlParentUuid.isEmpty()) {
            inCurrentFolder = true;
        }
    } else {

        if (incoming.source == BookSource::Xochitl
                && incoming.xochitlParentUuid == fk) {
            inCurrentFolder = true;
        }
    }

    if (inCurrentFolder) {
        const int row = m_books.size();
        beginInsertRows(QModelIndex(), row, row);
        m_books.append(incoming);
        endInsertRows();
    }
}

void LibraryModel::removeXochitlEntry(const QString &uuid)
{

    int allIdx = -1;
    for (int i = 0; i < m_allEntries.size(); ++i) {
        if (m_allEntries[i].xochitlUuid == uuid) {
            allIdx = i;
            break;
        }
    }
    if (allIdx < 0) {
        qDebug() << "LibraryModel: removeXochitlEntry  -  UUID not found:" << uuid;
        return;
    }
    m_allEntries.removeAt(allIdx);

    for (int i = 0; i < m_books.size(); ++i) {
        if (m_books[i].xochitlUuid == uuid) {
            beginRemoveRows(QModelIndex(), i, i);
            m_books.removeAt(i);
            endRemoveRows();
            break;
        }
    }

    qDebug() << "LibraryModel: xochitl entry removed" << uuid;
}

void LibraryModel::rebuildVisibleEntries()
{
    beginResetModel();
    m_books.clear();

    const QString &fk = m_currentFolderKey;

    for (const BookRecord &rec : std::as_const(m_allEntries)) {
        if (fk.isEmpty()) {

            if (rec.source == BookSource::CustomPath) {
                m_books.append(rec);
            } else if (rec.source == BookSource::Xochitl && rec.xochitlParentUuid.isEmpty()) {
                m_books.append(rec);
            }
        } else {

            if (rec.source == BookSource::Xochitl && rec.xochitlParentUuid == fk) {
                m_books.append(rec);
            }

        }
    }

    endResetModel();
}

void LibraryModel::updateContinueReading()
{
    const BookRecord *best = nullptr;
    for (const BookRecord &book : std::as_const(m_allEntries)) {
        if (book.isFolder) {
            continue;
        }
        if (book.progressPercent < 1 || book.progressPercent > 99) {
            continue;
        }
        if (!book.lastRead.isValid()) {
            continue;
        }
        if (!best || book.lastRead > best->lastRead) {
            best = &book;
        }
    }

    if (best) {
        m_lastReadBookPath           = best->filePath;
        m_lastReadBookTitle          = best->title;
        m_lastReadBookProgress       = best->progressPercent;
        m_lastReadBookCurrentChapter = best->currentChapter;
    } else {
        m_lastReadBookPath.clear();
        m_lastReadBookTitle.clear();
        m_lastReadBookProgress = 0;
        m_lastReadBookCurrentChapter.clear();
    }
}
