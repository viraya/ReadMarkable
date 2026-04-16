#pragma once

#include <QDateTime>
#include <QString>

struct XochitlEntry {

    QString   uuid;
    QString   visibleName;
    QString   type;
    QString   parent;

    bool      deleted = false;
    bool      tombstoneExists = false;

    QString   fileType;
    QString   author;

    QDateTime lastOpened;
    QDateTime lastModified;

    bool      isEpub = false;

    QString   epubFilePath;

    QString   thumbnailPath;

    int       epubDescendantCount = 0;
};
