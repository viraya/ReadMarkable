#pragma once

#include <QString>
#include <cstdint>

enum class AnnotationStyle : int {
    GrayFill   = 0,
    Underline  = 1,
    Bracket    = 2,
    DottedLine = 3,
    Yellow     = 4,
    Green      = 5,
    Blue       = 6,
    Red        = 7,
    Orange     = 8,
};

struct AnnotationEntry {
    int64_t         id           = 0;
    QString         bookSha;
    QString         startXPointer;
    QString         endXPointer;
    QString         selectedText;
    AnnotationStyle style = AnnotationStyle::GrayFill;
    QString         note;
    int64_t         createdAt    = 0;
    QString         chapterTitle;
};
