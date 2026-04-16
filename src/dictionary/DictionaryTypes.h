#pragma once
#include <QString>
#include <QList>

struct DictEntry {
    QString word;
    QString partOfSpeech;
    QString definition;
    QString ipa;
    QString example;
};

struct DictResult {
    QString         word;
    QList<DictEntry> entries;
    bool            found = false;
};
