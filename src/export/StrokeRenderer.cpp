#include "StrokeRenderer.h"

#include "annotation/MarginNoteService.h"

#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QVector>

#include <algorithm>
#include <cmath>

QByteArray StrokeRenderer::renderToPng(const QByteArray &strokeData, int width, int height)
{

    QVector<QVector<QPoint>> strokes = MarginNoteService::deserializeStrokes(strokeData);
    if (strokes.isEmpty()) {
        return QByteArray();
    }

    int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
    for (const QVector<QPoint> &stroke : strokes) {
        for (const QPoint &pt : stroke) {
            minX = std::min(minX, pt.x());
            minY = std::min(minY, pt.y());
            maxX = std::max(maxX, pt.x());
            maxY = std::max(maxY, pt.y());
        }
    }

    const int pad = 8;
    float sx = (maxX > minX) ? float(width  - 2 * pad) / float(maxX - minX) : 1.0f;
    float sy = (maxY > minY) ? float(height - 2 * pad) / float(maxY - minY) : 1.0f;
    float sc = std::min(sx, sy);

    QImage image(width, height, QImage::Format_Grayscale8);
    image.fill(0xFF);

    QPainter painter(&image);
    QPen pen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (const QVector<QPoint> &stroke : strokes) {
        if (stroke.size() < 2) {

            if (stroke.size() == 1) {
                const QPoint &pt = stroke[0];
                int sx2 = int(std::round((pt.x() - minX) * sc)) + pad;
                int sy2 = int(std::round((pt.y() - minY) * sc)) + pad;
                painter.drawPoint(sx2, sy2);
            }
            continue;
        }

        for (int i = 1; i < stroke.size(); ++i) {
            const QPoint &a = stroke[i - 1];
            const QPoint &b = stroke[i];

            int ax = int(std::round((a.x() - minX) * sc)) + pad;
            int ay = int(std::round((a.y() - minY) * sc)) + pad;
            int bx = int(std::round((b.x() - minX) * sc)) + pad;
            int by = int(std::round((b.y() - minY) * sc)) + pad;

            painter.drawLine(ax, ay, bx, by);
        }
    }

    painter.end();

    QByteArray out;
    QBuffer buf(&out);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "PNG");
    return out;
}
