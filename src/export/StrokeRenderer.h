#pragma once

#include <QByteArray>

class StrokeRenderer
{
public:
    StrokeRenderer() = delete;

    static QByteArray renderToPng(const QByteArray &strokeData,
                                  int width  = 400,
                                  int height = 400);
};
