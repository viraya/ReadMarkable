#include "PenInputController.h"
#include "PenReader.h"
#include "../annotation/SelectionController.h"
#include "../annotation/AnnotationService.h"
#include "../annotation/MarginNoteService.h"

#include <QDebug>
#include <QVariantMap>
#include <algorithm>

PenInputController::PenInputController(QObject *parent)
    : QObject(parent)
{
    qDebug() << "PenInputController: initialized";
}

void PenInputController::setDependencies(PenReader *penReader,
                                         SelectionController *selectionCtrl,
                                         AnnotationService *annotationSvc,
                                         MarginNoteService *marginNoteSvc)
{
    m_penReader     = penReader;
    m_selectionCtrl = selectionCtrl;
    m_annotationSvc = annotationSvc;
    m_marginNoteSvc = marginNoteSvc;
}

void PenInputController::setMarginNoteMode(bool active)
{
    m_marginCanvasActive = active;
    if (!active) {

        m_marginStrokes.clear();
    }
    qDebug() << "PenInputController: marginCanvasActive =" << active;
}

void PenInputController::setViewGeometry(int marginH, int marginV,
                                          int screenW, int screenH,
                                          qreal coordScaleX, qreal coordScaleY)
{
    m_marginH     = marginH;
    m_marginV     = marginV;
    m_screenW     = screenW;
    m_screenH     = screenH;
    m_coordScaleX = coordScaleX;
    m_coordScaleY = coordScaleY;
}

QVariantList PenInputController::inkTrail() const
{
    return m_inkTrail;
}

bool PenInputController::isDrawing() const
{
    return m_isDrawing;
}

QPointF PenInputController::hwToScreen(qreal penHwX, qreal penHwY) const
{
    if (!m_penReader || m_penReader->maxX() <= 0 || m_penReader->maxY() <= 0)
        return QPointF(penHwX, penHwY);

    qreal normX = penHwX / qreal(m_penReader->maxX());
    qreal normY = penHwY / qreal(m_penReader->maxY());
    return QPointF(normX * m_screenW, normY * m_screenH);
}

QPointF PenInputController::screenToContent(qreal screenX, qreal screenY) const
{
    qreal localX = screenX - m_marginH;
    qreal localY = screenY - m_marginV;
    return QPointF(localX * m_coordScaleX, localY * m_coordScaleY);
}

QString PenInputController::findParagraphXPointer(qreal contentX, qreal contentY) const
{
    if (!m_selectionCtrl)
        return QString();
    return m_selectionCtrl->getParagraphXPointer(
        static_cast<int>(contentX), static_cast<int>(contentY));
}

PenInputController::Mode PenInputController::determineMode(qreal ) const
{

    return TextHighlight;
}

void PenInputController::beginStroke(qreal penHwX, qreal penHwY)
{
    QPointF screen = hwToScreen(penHwX, penHwY);
    m_mode = determineMode(screen.x());

    m_isDrawing = true;
    emit isDrawingChanged();

    m_currentStroke.clear();
    m_inkTrail.clear();
    m_pointsSinceEmit = 0;

    if (m_mode == MarginNote && !m_marginCanvasActive) {
        m_marginStrokes.clear();
    }

    m_currentStroke.append(screen);

    QVariantMap dot;
    dot[QStringLiteral("x")]      = screen.x() - m_marginH;
    dot[QStringLiteral("y")]      = screen.y() - m_marginV;
    dot[QStringLiteral("width")]  = 4;
    dot[QStringLiteral("height")] = 4;
    m_inkTrail.append(dot);

    m_pointsSinceEmit = 1;
    if (m_pointsSinceEmit >= kEmitInterval) {
        m_pointsSinceEmit = 0;
        emit inkTrailChanged();
    }
}

void PenInputController::extendStroke(qreal penHwX, qreal penHwY)
{
    QPointF screen = hwToScreen(penHwX, penHwY);
    m_currentStroke.append(screen);

    QVariantMap dot;
    dot[QStringLiteral("x")]      = screen.x() - m_marginH;
    dot[QStringLiteral("y")]      = screen.y() - m_marginV;
    dot[QStringLiteral("width")]  = 4;
    dot[QStringLiteral("height")] = 4;
    m_inkTrail.append(dot);

    if (m_inkTrail.size() > kMaxTrailPoints) {

        for (int i = 0; i < 50; ++i)
            m_inkTrail.removeFirst();
    }

    ++m_pointsSinceEmit;
    if (m_pointsSinceEmit >= kEmitInterval) {
        m_pointsSinceEmit = 0;
        emit inkTrailChanged();
    }
}

void PenInputController::commitStroke()
{
    m_isDrawing = false;
    emit isDrawingChanged();

    clearInkTrail();

    if (m_mode == TextHighlight && !m_currentStroke.isEmpty() && m_selectionCtrl) {

        qreal minX = std::numeric_limits<qreal>::max();
        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxX = std::numeric_limits<qreal>::lowest();
        qreal maxY = std::numeric_limits<qreal>::lowest();

        for (const QPointF &screenPt : m_currentStroke) {
            QPointF contentPt = screenToContent(screenPt.x(), screenPt.y());
            if (contentPt.x() < minX) minX = contentPt.x();
            if (contentPt.y() < minY) minY = contentPt.y();
            if (contentPt.x() > maxX) maxX = contentPt.x();
            if (contentPt.y() > maxY) maxY = contentPt.y();
        }

        m_selectionCtrl->snapHighlightFromStrokeBounds(
            static_cast<int>(minX), static_cast<int>(minY),
            static_cast<int>(maxX), static_cast<int>(maxY));

    } else if (m_mode == Erase && !m_currentStroke.isEmpty() && m_selectionCtrl) {

        qreal minX = std::numeric_limits<qreal>::max();
        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxX = std::numeric_limits<qreal>::lowest();
        qreal maxY = std::numeric_limits<qreal>::lowest();

        for (const QPointF &screenPt : m_currentStroke) {
            QPointF contentPt = screenToContent(screenPt.x(), screenPt.y());
            if (contentPt.x() < minX) minX = contentPt.x();
            if (contentPt.y() < minY) minY = contentPt.y();
            if (contentPt.x() > maxX) maxX = contentPt.x();
            if (contentPt.y() > maxY) maxY = contentPt.y();
        }

        m_selectionCtrl->eraseHighlightsInStrokeBounds(
            static_cast<int>(minX), static_cast<int>(minY),
            static_cast<int>(maxX), static_cast<int>(maxY));

    } else if (m_mode == MarginNote && !m_currentStroke.isEmpty()) {

        const qreal textAreaRight = m_screenW * 0.85;

        QVector<QPoint> canvasStroke;
        canvasStroke.reserve(m_currentStroke.size());
        for (const QPointF &screenPt : m_currentStroke) {

            const int cx = static_cast<int>(screenPt.x() - textAreaRight);
            const int cy = static_cast<int>(screenPt.y() - m_marginV);

            canvasStroke.append(QPoint(
                qBound(-32768, cx, 32767),
                qBound(-32768, cy, 32767)));
        }

        m_marginStrokes.append(canvasStroke);

        const QPointF firstScreen = m_currentStroke.first();
        const QPointF firstContent = screenToContent(firstScreen.x(), firstScreen.y());
        const QString paragraphXPtr = findParagraphXPointer(firstContent.x(), firstContent.y());

        if (!paragraphXPtr.isEmpty()) {

            const QByteArray strokeData =
                MarginNoteService::serializeStrokes(m_marginStrokes);
            emit marginNoteStrokeCommitted(paragraphXPtr, strokeData);
            qDebug() << "PenInputController: margin note committed"
                     << m_marginStrokes.size() << "strokes, xptr:"
                     << paragraphXPtr.left(40);
        } else {
            qDebug() << "PenInputController: margin note  -  no paragraph at stroke origin, discarding";
        }
    }

    m_mode = Idle;
    m_currentStroke.clear();
}

void PenInputController::clearInkTrail()
{
    m_inkTrail.clear();
    emit inkTrailChanged();
}

void PenInputController::onPenEvent()
{
    if (!m_penReader)
        return;

    bool nowDown = m_penReader->penDown();
    qreal penHwX = m_penReader->penX();
    qreal penHwY = m_penReader->penY();

    bool eraser = m_penReader->isEraser();

    if (!m_wasDown && nowDown) {

        beginStroke(penHwX, penHwY);
        if (eraser)
            m_mode = Erase;
    } else if (m_wasDown && !nowDown) {

        commitStroke();
    } else if (nowDown) {

        extendStroke(penHwX, penHwY);
    }

    m_wasDown = nowDown;
}
