#pragma once

#include "BookRecord.h"
#include "XochitlEntry.h"

#include <QAbstractListModel>
#include <QList>
#include <QVector>

class XochitlLibrarySource;

class LibraryModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString lastReadBookPath           READ lastReadBookPath           NOTIFY scanFinished)
    Q_PROPERTY(QString lastReadBookTitle          READ lastReadBookTitle          NOTIFY scanFinished)
    Q_PROPERTY(int     lastReadBookProgress       READ lastReadBookProgress       NOTIFY scanFinished)
    Q_PROPERTY(QString lastReadBookCurrentChapter READ lastReadBookCurrentChapter NOTIFY scanFinished)
    Q_PROPERTY(bool    isLibraryEmpty             READ isLibraryEmpty             NOTIFY scanFinished)

public:

    enum BookRole {
        FilePathRole      = Qt::UserRole + 1,
        TitleRole,
        AuthorRole,
        CoverKeyRole,
        ProgressPercentRole,
        LastReadRole,
        AddedDateRole,
        CurrentChapterRole,
        IsFolderRole,
        IsEmptyRole,
        FolderPathRole,
        BookCountRole,
        DescriptionRole,
        XochitlUuidRole,
    };
    Q_ENUM(BookRole)

    explicit LibraryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString lastReadBookPath()     const { return m_lastReadBookPath; }
    QString lastReadBookTitle()    const { return m_lastReadBookTitle; }
    int     lastReadBookProgress() const { return m_lastReadBookProgress; }
    QString lastReadBookCurrentChapter() const { return m_lastReadBookCurrentChapter; }

    bool isLibraryEmpty() const { return m_customScanDone && m_xochitlScanDone && m_allEntries.isEmpty(); }

    void setXochitlSource(XochitlLibrarySource *src);

    Q_INVOKABLE void setCurrentlyOpenBookPath(const QString &path);

    Q_INVOKABLE void setCurrentFolder(const QString &folderKey);

    Q_INVOKABLE void addBook(const BookRecord &record);

    Q_INVOKABLE void onScanComplete();

    Q_INVOKABLE void clear();

    Q_INVOKABLE int count() const;

    Q_INVOKABLE void rescan();

    Q_INVOKABLE void rescanDirectory(const QString &path);

    Q_INVOKABLE void refreshPositions();

    void setPositionRepository(class PositionRepository *repo) { m_positionRepo = repo; }

public slots:

    void onXochitlEntryAdded(const XochitlEntry &entry);
    void onXochitlEntryRemoved(const QString &uuid);
    void onXochitlScanComplete();

signals:

    void scanFinished();

    void rescanRequested();

    void rescanDirectoryRequested(const QString &path);

    void currentBookExternallyDeleted(const QString &filePath);

private:

    BookRecord xochitlEntryToBookRecord(const XochitlEntry &entry) const;

    void mergeEntry(const BookRecord &incoming);

    void removeXochitlEntry(const QString &uuid);

    void rebuildVisibleEntries();

    void updateContinueReading();

    QVector<BookRecord> m_books;

    QList<BookRecord>   m_allEntries;

    QString m_currentFolderKey;

    XochitlLibrarySource *m_xochitl       = nullptr;
    class PositionRepository *m_positionRepo = nullptr;

    bool m_customScanDone  = false;
    bool m_xochitlScanDone = false;

    QString m_currentlyOpenBookPath;

    QString m_lastReadBookPath;
    QString m_lastReadBookTitle;
    int     m_lastReadBookProgress = 0;
    QString m_lastReadBookCurrentChapter;
};
