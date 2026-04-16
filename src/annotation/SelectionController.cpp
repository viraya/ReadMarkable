#include "SelectionController.h"
#include "AnnotationService.h"
#include "../epub/CrEngineRenderer.h"

#include "lvdocview.h"
#include "lvtinydom.h"

#include <QDebug>
#include <QRect>
#include <QVariantMap>

SelectionController::SelectionController(CrEngineRenderer  *renderer,
                                         AnnotationService *annotationService,
                                         QObject           *parent)
    : QObject(parent)
    , m_renderer(renderer)
    , m_annotationService(annotationService)
{
    Q_ASSERT(renderer);
    Q_ASSERT(annotationService);
    qDebug() << "SelectionController: initialized";
}

SelectionController::~SelectionController() = default;

LVDocView *SelectionController::docView() const
{
    if (!m_renderer || !m_renderer->isLoaded())
        return nullptr;
    return m_renderer->docView();
}

QVariantList SelectionController::computeSelectionRects(ldomXRange &range)
{
    LVDocView *dv = docView();
    if (!dv)
        return {};

    LVArray<lvRect> docRects;
    range.getSegmentRects(docRects);

    QVariantList result;
    result.reserve(docRects.length());

    for (int i = 0; i < docRects.length(); ++i) {
        const lvRect &dr = docRects[i];

        lvPoint topLeft(dr.left, dr.top);
        if (!dv->docToWindowPoint(topLeft, false))
            continue;

        lvPoint bottomRight(dr.right, dr.bottom);
        if (!dv->docToWindowPoint(bottomRight, true))
            continue;

        if (bottomRight.x <= topLeft.x || bottomRight.y <= topLeft.y)
            continue;

        result.append(QVariant::fromValue(
            QRect(topLeft.x, topLeft.y,
                  bottomRight.x - topLeft.x,
                  bottomRight.y - topLeft.y)));
    }

    return result;
}

void SelectionController::updateHandlePositions()
{
    if (m_rects.isEmpty()) {
        m_startHandleX = m_startHandleY = 0;
        m_endHandleX   = m_endHandleY   = 0;
        return;
    }

    const QRect firstRect = m_rects.first().toRect();
    m_startHandleX = firstRect.left();
    m_startHandleY = firstRect.bottom();

    const QRect lastRect = m_rects.last().toRect();
    m_endHandleX = lastRect.right();
    m_endHandleY = lastRect.bottom();
}

void SelectionController::startSelectionAt(int contentX, int contentY)
{
    LVDocView *dv = docView();
    if (!dv)
        return;

    lvPoint pt(contentX, contentY);

    ldomXPointer ptr = dv->getNodeByPoint(pt, false, true);
    if (ptr.isNull()) {
        qDebug() << "SelectionController::startSelectionAt: no node at" << contentX << contentY;
        return;
    }

    ldomXRange wordRange;
    if (!ldomXRange::getWordRange(wordRange, ptr)) {
        qDebug() << "SelectionController::startSelectionAt: getWordRange failed";
        return;
    }

    lString32 text = wordRange.getRangeText();
    m_selectedText = QString::fromUtf8(UnicodeToUtf8(text).c_str());

    lString32 startStr = wordRange.getStart().toString();
    lString32 endStr   = wordRange.getEnd().toString();
    m_startXPtr = QString::fromUtf8(UnicodeToUtf8(startStr).c_str());
    m_endXPtr   = QString::fromUtf8(UnicodeToUtf8(endStr).c_str());

    m_rects = computeSelectionRects(wordRange);
    updateHandlePositions();

    m_hasSelection = true;
    m_activeAnnotationId = 0;
    emit selectionChanged();

    qDebug() << "SelectionController: selected word" << m_selectedText
             << "rects:" << m_rects.size();
}

void SelectionController::extendSelectionTo(int contentX, int contentY, bool isStartHandle)
{
    if (!m_hasSelection)
        return;

    LVDocView *dv = docView();
    if (!dv)
        return;

    ldomDocument *dom = dv->getDocument();
    if (!dom)
        return;

    lvPoint pt(contentX, contentY);
    ldomXPointer dragPtr = dv->getNodeByPoint(pt, false, true);
    if (dragPtr.isNull())
        return;

    const QString &anchorStr = isStartHandle ? m_endXPtr : m_startXPtr;
    if (anchorStr.isEmpty())
        return;

    const std::string anchorUtf8 = anchorStr.toStdString();
    ldomXPointer anchorPtr = dom->createXPointer(Utf8ToUnicode(anchorUtf8.c_str()));
    if (anchorPtr.isNull())
        return;

    ldomXRange newRange = isStartHandle
        ? ldomXRange(dragPtr, anchorPtr)
        : ldomXRange(anchorPtr, dragPtr);
    newRange.sort();

    if (isStartHandle) {
        lString32 s = newRange.getStart().toString();
        m_startXPtr = QString::fromUtf8(UnicodeToUtf8(s).c_str());
    } else {
        lString32 e = newRange.getEnd().toString();
        m_endXPtr = QString::fromUtf8(UnicodeToUtf8(e).c_str());
    }

    lString32 text = newRange.getRangeText();
    m_selectedText = QString::fromUtf8(UnicodeToUtf8(text).c_str());

    m_rects = computeSelectionRects(newRange);
    updateHandlePositions();

    m_activeAnnotationId = 0;

    emit selectionChanged();
}

void SelectionController::clearSelection()
{
    m_hasSelection        = false;
    m_rects.clear();
    m_selectedText.clear();
    m_startXPtr.clear();
    m_endXPtr.clear();
    m_startHandleX       = 0;
    m_startHandleY       = 0;
    m_endHandleX         = 0;
    m_endHandleY         = 0;
    m_activeAnnotationId = 0;
    emit selectionChanged();
}

void SelectionController::loadHighlightsForPage()
{
    LVDocView *dv = docView();
    if (!dv || !m_annotationService) {
        m_pageHighlights.clear();
        emit pageHighlightsChanged();
        return;
    }

    ldomDocument *dom = dv->getDocument();
    if (!dom) {
        m_pageHighlights.clear();
        emit pageHighlightsChanged();
        return;
    }

    const QVariantList annotations = m_annotationService->annotationsForCurrentBook();

    QVariantList result;
    result.reserve(annotations.size());

    for (const QVariant &v : annotations) {
        const QVariantMap ann = v.toMap();
        const QString startStr = ann.value(QStringLiteral("startXPointer")).toString();
        const QString endStr   = ann.value(QStringLiteral("endXPointer")).toString();
        const int     style    = ann.value(QStringLiteral("style")).toInt();
        const int     id       = ann.value(QStringLiteral("id")).toInt();

        if (startStr.isEmpty() || endStr.isEmpty())
            continue;

        ldomXPointer startPtr = dom->createXPointer(
            Utf8ToUnicode(startStr.toStdString().c_str()));
        ldomXPointer endPtr   = dom->createXPointer(
            Utf8ToUnicode(endStr.toStdString().c_str()));

        if (startPtr.isNull() || endPtr.isNull())
            continue;

        ldomXRange range(startPtr, endPtr);

        QVariantList rects = computeSelectionRects(range);
        if (rects.isEmpty())
            continue;

        QVariantMap group;
        group[QStringLiteral("rects")]        = rects;
        group[QStringLiteral("style")]        = style;
        group[QStringLiteral("annotationId")] = id;
        result.append(group);
    }

    m_pageHighlights = result;
    emit pageHighlightsChanged();

    qDebug() << "SelectionController::loadHighlightsForPage:"
             << result.size() << "highlight groups visible on current page";
}

void SelectionController::snapHighlightFromStrokeBounds(int contentLeft, int contentTop,
                                                         int contentRight, int contentBottom)
{
    LVDocView *dv = docView();
    if (!dv || !m_annotationService)
        return;

    ldomDocument *dom = dv->getDocument();
    if (!dom)
        return;

    lvPoint ptTopLeft(contentLeft, contentTop);
    lvPoint ptBottomRight(contentRight, contentBottom);

    ldomXPointer ptrStart = dv->getNodeByPoint(ptTopLeft, false, true);
    ldomXPointer ptrEnd   = dv->getNodeByPoint(ptBottomRight, false, true);

    if (ptrStart.isNull() || ptrEnd.isNull())
        return;

    ldomXRange strokeRange(ptrStart, ptrEnd);
    strokeRange.sort();

    LVArray<lvRect> docRects;
    strokeRange.getSegmentRects(docRects);

    bool hasVisibleRect = false;
    for (int i = 0; i < docRects.length(); ++i) {
        const lvRect &dr = docRects[i];
        lvPoint tl(dr.left, dr.top);
        if (!dv->docToWindowPoint(tl, false))
            continue;
        lvPoint br(dr.right, dr.bottom);
        if (!dv->docToWindowPoint(br, true))
            continue;
        if (br.x <= tl.x || br.y <= tl.y)
            continue;
        if (tl.y < contentBottom && br.y > contentTop) {
            hasVisibleRect = true;
            break;
        }
    }
    if (!hasVisibleRect)
        return;

    lvPoint strokeDocStart = strokeRange.getStart().toPoint();
    lvPoint strokeDocEnd   = strokeRange.getEnd().toPoint();
    int strokeYMin = qMin(strokeDocStart.y, strokeDocEnd.y);
    int strokeYMax = qMax(strokeDocStart.y, strokeDocEnd.y);

    ldomXPointer unionStart = strokeRange.getStart();
    ldomXPointer unionEnd   = strokeRange.getEnd();
    QVector<int64_t> overlappingIds;
    int bestStyle = 0;

    const auto &cached = m_annotationService->cachedAnnotations();
    for (const auto &ann : cached) {
        if (ann.startXPointer.isEmpty() || ann.endXPointer.isEmpty())
            continue;

        ldomXPointer annStart = dom->createXPointer(
            Utf8ToUnicode(ann.startXPointer.toStdString().c_str()));
        ldomXPointer annEnd = dom->createXPointer(
            Utf8ToUnicode(ann.endXPointer.toStdString().c_str()));
        if (annStart.isNull() || annEnd.isNull())
            continue;

        lvPoint annDocStart = annStart.toPoint();
        lvPoint annDocEnd   = annEnd.toPoint();
        int annYMin = qMin(annDocStart.y, annDocEnd.y);
        int annYMax = qMax(annDocStart.y, annDocEnd.y);

        bool overlaps = (annYMin <= strokeYMax + 5) && (annYMax >= strokeYMin - 5);
        if (!overlaps)
            continue;

        overlappingIds.append(ann.id);
        if (overlappingIds.size() == 1)
            bestStyle = static_cast<int>(ann.style);

        if (annDocStart.y < unionStart.toPoint().y ||
            (annDocStart.y == unionStart.toPoint().y && annDocStart.x < unionStart.toPoint().x))
            unionStart = annStart;
        if (annDocEnd.y > unionEnd.toPoint().y ||
            (annDocEnd.y == unionEnd.toPoint().y && annDocEnd.x > unionEnd.toPoint().x))
            unionEnd = annEnd;
    }

    if (overlappingIds.size() == 1) {
        const auto &ann = cached[0];
        for (const auto &a : cached) {
            if (a.id == overlappingIds[0]) {
                ldomXPointer aStart = dom->createXPointer(
                    Utf8ToUnicode(a.startXPointer.toStdString().c_str()));
                ldomXPointer aEnd = dom->createXPointer(
                    Utf8ToUnicode(a.endXPointer.toStdString().c_str()));
                if (!aStart.isNull() && !aEnd.isNull()) {
                    lvPoint as = aStart.toPoint();
                    lvPoint ae = aEnd.toPoint();
                    int aMin = qMin(as.y, ae.y);
                    int aMax = qMax(as.y, ae.y);

                    if (strokeYMin >= aMin - 5 && strokeYMax <= aMax + 5) {
                        qDebug() << "SelectionController: pen stroke within existing highlight, no-op";
                        return;
                    }
                }
                break;
            }
        }
    }

    ldomXRange finalRange(unionStart, unionEnd);
    finalRange.sort();

    lString32 finalStartStr = finalRange.getStart().toString();
    lString32 finalEndStr   = finalRange.getEnd().toString();
    QString finalStartQ = QString::fromUtf8(UnicodeToUtf8(finalStartStr).c_str());
    QString finalEndQ   = QString::fromUtf8(UnicodeToUtf8(finalEndStr).c_str());

    if (finalStartQ.isEmpty() || finalEndQ.isEmpty())
        return;

    lString32 rangeText = finalRange.getRangeText();
    QString selectedText = QString::fromUtf8(UnicodeToUtf8(rangeText).c_str());

    for (int64_t id : overlappingIds) {
        m_annotationService->deleteAnnotation(static_cast<int>(id));
    }

    int style = overlappingIds.isEmpty() ? 0 : bestStyle;
    m_annotationService->saveHighlight(finalStartQ, finalEndQ, selectedText, style, "");

    loadHighlightsForPage();

    qDebug() << "SelectionController: pen snap highlight"
             << selectedText.left(50)
             << "merged:" << overlappingIds.size() << "existing";
}

void SelectionController::eraseHighlightsInStrokeBounds(int contentLeft, int contentTop,
                                                         int contentRight, int contentBottom)
{
    LVDocView *dv = docView();
    if (!dv || !m_annotationService)
        return;

    ldomDocument *dom = dv->getDocument();
    if (!dom)
        return;

    ldomXPointer ptrStart = dv->getNodeByPoint(lvPoint(contentLeft, contentTop), false, true);
    ldomXPointer ptrEnd   = dv->getNodeByPoint(lvPoint(contentRight, contentBottom), false, true);
    if (ptrStart.isNull() || ptrEnd.isNull())
        return;

    lvPoint strokeDocStart = ptrStart.toPoint();
    lvPoint strokeDocEnd   = ptrEnd.toPoint();
    int strokeYMin = qMin(strokeDocStart.y, strokeDocEnd.y);
    int strokeYMax = qMax(strokeDocStart.y, strokeDocEnd.y);

    QVector<int64_t> toDelete;
    const auto &cached = m_annotationService->cachedAnnotations();
    for (const auto &ann : cached) {
        if (ann.startXPointer.isEmpty() || ann.endXPointer.isEmpty())
            continue;

        ldomXPointer annStart = dom->createXPointer(
            Utf8ToUnicode(ann.startXPointer.toStdString().c_str()));
        ldomXPointer annEnd = dom->createXPointer(
            Utf8ToUnicode(ann.endXPointer.toStdString().c_str()));
        if (annStart.isNull() || annEnd.isNull())
            continue;

        lvPoint annDocStart = annStart.toPoint();
        lvPoint annDocEnd   = annEnd.toPoint();
        int annYMin = qMin(annDocStart.y, annDocEnd.y);
        int annYMax = qMax(annDocStart.y, annDocEnd.y);

        if (annYMin <= strokeYMax + 5 && annYMax >= strokeYMin - 5)
            toDelete.append(ann.id);
    }

    if (toDelete.isEmpty())
        return;

    for (int64_t id : toDelete)
        m_annotationService->deleteAnnotation(static_cast<int>(id));

    loadHighlightsForPage();

    qDebug() << "SelectionController: eraser deleted" << toDelete.size() << "highlights";
}

QString SelectionController::getParagraphXPointer(int contentX, int contentY)
{
    LVDocView *dv = docView();
    if (!dv)
        return QString();

    lvPoint pt(contentX, contentY);

    ldomXPointer ptr = dv->getNodeByPoint(pt, false, true);
    if (ptr.isNull()) {
        qDebug() << "SelectionController::getParagraphXPointer: no node at"
                 << contentX << contentY;
        return QString();
    }

    ldomNode *node = ptr.getNode();
    while (node && !node->isNull()) {

        if (node->isElement()) {
            int rendMethod = node->getRendMethod();

            if (rendMethod == 0  || rendMethod == 5 ) {
                ldomXPointer blockPtr(node, 0);
                lString32 xptrStr = blockPtr.toString();
                const QString result =
                    QString::fromUtf8(UnicodeToUtf8(xptrStr).c_str());
                if (!result.isEmpty())
                    return result;
            }
        }
        ldomNode *parent = node->getParentNode();
        if (!parent || parent == node)
            break;
        node = parent;
    }

    lString32 fallback = ptr.toString();
    return QString::fromUtf8(UnicodeToUtf8(fallback).c_str());
}

void SelectionController::reactivateHighlight(int annotationId)
{
    if (!m_annotationService)
        return;

    LVDocView *dv = docView();
    if (!dv)
        return;

    ldomDocument *dom = dv->getDocument();
    if (!dom)
        return;

    const QVariantList annotations = m_annotationService->annotationsForCurrentBook();
    QVariantMap found;
    for (const QVariant &v : annotations) {
        const QVariantMap ann = v.toMap();
        if (ann.value(QStringLiteral("id")).toInt() == annotationId) {
            found = ann;
            break;
        }
    }

    if (found.isEmpty()) {
        qWarning() << "SelectionController::reactivateHighlight: annotation"
                   << annotationId << "not found";
        return;
    }

    const QString startStr = found.value(QStringLiteral("startXPointer")).toString();
    const QString endStr   = found.value(QStringLiteral("endXPointer")).toString();

    ldomXPointer startPtr = dom->createXPointer(
        Utf8ToUnicode(startStr.toStdString().c_str()));
    ldomXPointer endPtr   = dom->createXPointer(
        Utf8ToUnicode(endStr.toStdString().c_str()));

    if (startPtr.isNull() || endPtr.isNull()) {
        qWarning() << "SelectionController::reactivateHighlight: invalid XPointers for"
                   << annotationId;
        return;
    }

    ldomXRange range(startPtr, endPtr);

    m_startXPtr    = startStr;
    m_endXPtr      = endStr;
    m_selectedText = found.value(QStringLiteral("selectedText")).toString();
    m_rects        = computeSelectionRects(range);
    updateHandlePositions();
    m_hasSelection       = true;
    m_activeAnnotationId = annotationId;

    emit selectionChanged();

    qDebug() << "SelectionController::reactivateHighlight: restored annotation"
             << annotationId << "rects:" << m_rects.size();
}
