#include "LibraryScanner.h"

#include "epub/EpubParser.h"
#include "epub/PositionRepository.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>

LibraryScanner::LibraryScanner(const QString      &libraryPath,
                               PositionRepository *positionRepo,
                               QObject            *parent)
    : QObject(parent)
    , m_libraryPath(libraryPath)
    , m_positionRepo(positionRepo)
{

    qRegisterMetaType<BookRecord>("BookRecord");
}

void LibraryScanner::scan()
{
    qDebug() << "LibraryScanner: starting recursive scan of" << m_libraryPath;

    const QDir rootDir(m_libraryPath);
    if (!rootDir.exists()) {
        qWarning() << "LibraryScanner: library path does not exist  - " << m_libraryPath;
        emit scanComplete();
        return;
    }

    QDirIterator fileIt(
        m_libraryPath,
        QStringList{ QStringLiteral("*.epub") },
        QDir::Files | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories
    );

    while (fileIt.hasNext()) {
        const QString epubPath = fileIt.next();
        emit bookFound(buildBookRecord(epubPath));
    }

    const QStringList subDirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &dirName : subDirs) {
        const QString dirPath = rootDir.absoluteFilePath(dirName);
        const int epubCount   = countEpubsInDirectory(dirPath);

        if (epubCount == 0) {
            continue;
        }

        BookRecord folder;
        folder.isFolder   = true;
        folder.folderPath = dirPath;
        folder.title      = dirName;
        folder.bookCount  = epubCount;
        folder.addedDate  = QFileInfo(dirPath).lastModified();

        emit bookFound(folder);
    }

    qDebug() << "LibraryScanner: scan finished";
    emit scanComplete();
}

void LibraryScanner::scanDirectory(const QString &dirPath)
{
    qDebug() << "LibraryScanner: scanning directory" << dirPath;

    const QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "LibraryScanner::scanDirectory: path does not exist  - " << dirPath;
        emit scanComplete();
        return;
    }

    const QStringList epubFiles = dir.entryList(
        QStringList{ QStringLiteral("*.epub") },
        QDir::Files | QDir::NoDotAndDotDot
    );

    for (const QString &fileName : epubFiles) {
        const QString epubPath = dir.absoluteFilePath(fileName);
        emit bookFound(buildBookRecord(epubPath));
    }

    const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subDirName : subDirs) {
        const QString subDirPath = dir.absoluteFilePath(subDirName);
        const int epubCount      = countEpubsInDirectory(subDirPath);

        if (epubCount == 0) {
            continue;
        }

        BookRecord folder;
        folder.isFolder   = true;
        folder.folderPath = subDirPath;
        folder.title      = subDirName;
        folder.bookCount  = epubCount;
        folder.addedDate  = QFileInfo(subDirPath).lastModified();

        emit bookFound(folder);
    }

    qDebug() << "LibraryScanner::scanDirectory: finished";
    emit scanComplete();
}

BookRecord LibraryScanner::buildBookRecord(const QString &epubPath) const
{
    BookRecord record;
    record.filePath  = epubPath;
    record.addedDate = QFileInfo(epubPath).lastModified();

    const EpubDocument doc = EpubParser::parse(epubPath);
    if (doc.isValid()) {
        record.title         = doc.title.isEmpty()
                                   ? QFileInfo(epubPath).baseName()
                                   : doc.title;
        record.author        = doc.author;
        record.description   = doc.description;
        record.coverImagePath= doc.coverImagePath;
    } else {
        qWarning() << "LibraryScanner: could not parse EPUB metadata for"
                   << QFileInfo(epubPath).fileName()
                   << " -  using filename fallback";
        record.title         = QFileInfo(epubPath).baseName();
        record.author        = QString();
        record.coverImagePath= QString();
    }

    if (m_positionRepo) {
        record.progressPercent = m_positionRepo->loadProgressPercent(epubPath);
        record.lastRead        = m_positionRepo->loadLastReadTimestamp(epubPath);
        record.currentChapter  = m_positionRepo->loadCurrentChapter(epubPath);
    }

    return record;
}

int LibraryScanner::countEpubsInDirectory(const QString &dirPath)
{
    int count = 0;
    QDirIterator it(
        dirPath,
        QStringList{ QStringLiteral("*.epub") },
        QDir::Files | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories
    );
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}
