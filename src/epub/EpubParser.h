#pragma once

#include "EpubDocument.h"
#include <QString>

class EpubParser {
public:

    static EpubDocument parse(const QString &epubPath);

private:
    EpubParser() = delete;
};
