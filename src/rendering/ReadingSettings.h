#pragma once

#include <QString>

struct ReadingSettings {
    QString fontFace    = QStringLiteral("Noto Serif");
    int     fontSize    = 14;
    int     marginLeft  = 48;
    int     marginTop   = 64;
    int     marginRight = 48;
    int     marginBottom= 64;
    int     lineSpacing = 100;
    int     viewWidth   = 954;
    int     viewHeight  = 1696;

    bool operator==(const ReadingSettings &o) const
    {
        return fontFace    == o.fontFace
            && fontSize    == o.fontSize
            && marginLeft  == o.marginLeft
            && marginTop   == o.marginTop
            && marginRight == o.marginRight
            && marginBottom== o.marginBottom
            && lineSpacing == o.lineSpacing
            && viewWidth   == o.viewWidth
            && viewHeight  == o.viewHeight;
    }

    bool operator!=(const ReadingSettings &o) const { return !(*this == o); }
};
