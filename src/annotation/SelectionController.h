#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QRect>

class CrEngineRenderer;
class LVDocView;
class ldomXRange;

class AnnotationService;

class SelectionController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool         hasSelection  READ hasSelection  NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectionRects READ selectionRects NOTIFY selectionChanged)
    Q_PROPERTY(QString      selectedText  READ selectedText  NOTIFY selectionChanged)
    Q_PROPERTY(QString      startXPointer READ startXPointer NOTIFY selectionChanged)
    Q_PROPERTY(QString      endXPointer   READ endXPointer   NOTIFY selectionChanged)

    Q_PROPERTY(int startHandleX READ startHandleX NOTIFY selectionChanged)
    Q_PROPERTY(int startHandleY READ startHandleY NOTIFY selectionChanged)
    Q_PROPERTY(int endHandleX   READ endHandleX   NOTIFY selectionChanged)
    Q_PROPERTY(int endHandleY   READ endHandleY   NOTIFY selectionChanged)

    Q_PROPERTY(QVariantList pageHighlightRects READ pageHighlightRects NOTIFY pageHighlightsChanged)

public:

    explicit SelectionController(CrEngineRenderer  *renderer,
                                 AnnotationService *annotationService,
                                 QObject           *parent = nullptr);
    ~SelectionController() override;

    bool         hasSelection()        const { return m_hasSelection; }
    QVariantList selectionRects()      const { return m_rects; }
    QString      selectedText()        const { return m_selectedText; }
    QString      startXPointer()       const { return m_startXPtr; }
    QString      endXPointer()         const { return m_endXPtr; }
    int          startHandleX()        const { return m_startHandleX; }
    int          startHandleY()        const { return m_startHandleY; }
    int          endHandleX()          const { return m_endHandleX; }
    int          endHandleY()          const { return m_endHandleY; }
    QVariantList pageHighlightRects()  const { return m_pageHighlights; }

    Q_INVOKABLE void startSelectionAt(int contentX, int contentY);

    Q_INVOKABLE void extendSelectionTo(int contentX, int contentY, bool isStartHandle);

    Q_INVOKABLE void clearSelection();

    Q_INVOKABLE void loadHighlightsForPage();

    Q_INVOKABLE void reactivateHighlight(int annotationId);

    Q_INVOKABLE int activeAnnotationId() const { return m_activeAnnotationId; }

    Q_INVOKABLE void snapHighlightFromStrokeBounds(int contentLeft, int contentTop,
                                                    int contentRight, int contentBottom);

    Q_INVOKABLE void eraseHighlightsInStrokeBounds(int contentLeft, int contentTop,
                                                    int contentRight, int contentBottom);

    Q_INVOKABLE QString getParagraphXPointer(int contentX, int contentY);

signals:

    void selectionChanged();

    void pageHighlightsChanged();

private:

    LVDocView *docView() const;

    QVariantList computeSelectionRects(ldomXRange &range);

    void updateHandlePositions();

    CrEngineRenderer  *m_renderer;
    AnnotationService *m_annotationService;

    bool         m_hasSelection       = false;
    QVariantList m_rects;
    QString      m_selectedText;
    QString      m_startXPtr;
    QString      m_endXPtr;
    int          m_startHandleX       = 0;
    int          m_startHandleY       = 0;
    int          m_endHandleX         = 0;
    int          m_endHandleY         = 0;

    QVariantList m_pageHighlights;

    int          m_activeAnnotationId = 0;
};
