#pragma once

#include <QString>
#include <QVector>

struct SpineItem {
    QString id;
    QString href;
    QString mediaType;
    bool linear;
};

struct TocEntry {
    QString label;
    QString href;
    int level;
    QVector<TocEntry> children;
};

struct EpubDocument {
    QString filePath;
    QString title;
    QString author;
    QString language;
    QString description;
    QString coverImagePath;

    QVector<SpineItem> spine;
    QVector<TocEntry>  toc;

    bool isValid() const {
        return !filePath.isEmpty() && !spine.isEmpty();
    }
};
