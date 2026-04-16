#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QMetaType>
#include <QString>

enum class BookSource : quint8 {
    CustomPath = 0,
    Xochitl    = 1,
};

struct BookRecord
{

    QString  filePath;
    QString  title;
    QString  author;
    QString  description;
    QString  coverImagePath;

    int      progressPercent = 0;
    QDateTime lastRead;
    QDateTime addedDate;
    QString  currentChapter;

    bool     isFolder  = false;
    bool     isEmpty   = false;
    QString  folderPath;
    int      bookCount = 0;

    BookSource source          = BookSource::CustomPath;
    QString    xochitlUuid;
    QString    xochitlParentUuid;
};

Q_DECLARE_METATYPE(BookRecord)
