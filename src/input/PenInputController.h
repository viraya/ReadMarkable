#pragma once
#include <QByteArray>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QVariantList>
#include <QVector>

class PenReader;
class SelectionController;
class AnnotationService;
class MarginNoteService;

class PenInputController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList inkTrail READ inkTrail NOTIFY inkTrailChanged)
    Q_PROPERTY(bool isDrawing READ isDrawing NOTIFY isDrawingChanged)

public:
    explicit PenInputController(QObject *parent = nullptr);

    void setDependencies(PenReader *penReader,
                         SelectionController *selectionCtrl,
                         AnnotationService *annotationSvc,
                         MarginNoteService *marginNoteSvc);

    Q_INVOKABLE void setViewGeometry(int marginH, int marginV,
                                     int screenW, int screenH,
                                     qreal coordScaleX, qreal coordScaleY);

    Q_INVOKABLE void setMarginNoteMode(bool active);

    QVariantList inkTrail() const;
    bool isDrawing() const;

signals:
    void inkTrailChanged();
    void isDrawingChanged();

    void marginNoteStrokeCommitted(const QString &paragraphXPtr,
                                   const QByteArray &strokeData);

public slots:
    void onPenEvent();

private:
    enum Mode { Idle, TextHighlight, MarginNote, Erase };

    void beginStroke(qreal penHwX, qreal penHwY);
    void extendStroke(qreal penHwX, qreal penHwY);
    void commitStroke();
    void clearInkTrail();

    QPointF hwToScreen(qreal penHwX, qreal penHwY) const;
    QPointF screenToContent(qreal screenX, qreal screenY) const;
    Mode determineMode(qreal screenX) const;

    QString findParagraphXPointer(qreal contentX, qreal contentY) const;

    PenReader           *m_penReader       = nullptr;
    SelectionController *m_selectionCtrl   = nullptr;
    AnnotationService   *m_annotationSvc   = nullptr;
    MarginNoteService   *m_marginNoteSvc   = nullptr;

    Mode m_mode      = Idle;
    bool m_isDrawing = false;
    bool m_wasDown   = false;
    QVector<QPointF> m_currentStroke;
    QVariantList m_inkTrail;

    QVector<QVector<QPoint>> m_marginStrokes;

    bool m_marginCanvasActive = false;

    int   m_marginH     = 48,  m_marginV     = 64;
    int   m_screenW     = 954, m_screenH     = 1696;
    qreal m_coordScaleX = 1.0, m_coordScaleY = 1.0;

    int m_pointsSinceEmit = 0;
    static constexpr int kEmitInterval    = 1;

    static constexpr int kMaxTrailPoints  = 200;
};
