#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <QStringList>

class LVDocView;

class CrEngineRenderer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int     pageCount     READ pageCount     NOTIFY pageCountChanged)
    Q_PROPERTY(int     currentPage   READ currentPage   NOTIFY currentPageChanged)
    Q_PROPERTY(bool    isLoaded      READ isLoaded      NOTIFY loadedChanged)
    Q_PROPERTY(bool    isLandscape   READ isLandscape   NOTIFY orientationChanged)
    Q_PROPERTY(int     renderVersion READ renderVersion NOTIFY renderVersionChanged)
    Q_PROPERTY(QString documentPath  READ documentPath  NOTIFY loadedChanged)

public:
    explicit CrEngineRenderer(QObject *parent = nullptr);
    ~CrEngineRenderer() override;

    Q_INVOKABLE bool loadDocument(const QString &epubPath);

    Q_INVOKABLE void closeDocument();

    bool isLoaded() const;

    QString documentPath() const;

    Q_INVOKABLE QImage renderPage(int pageNumber);

    Q_INVOKABLE QImage renderCurrentPage();

    int pageCount() const;

    int currentPage() const;

    Q_INVOKABLE void goToPage(int page);

    Q_INVOKABLE QString currentXPointer() const;

    Q_INVOKABLE void goToXPointer(const QString &xpointer);

    Q_INVOKABLE void setFontFace(const QString &fontName);
    Q_INVOKABLE void setFontSize(int ptSize);
    Q_INVOKABLE void setPageMargins(int left, int top, int right, int bottom);
    Q_INVOKABLE void setLineSpacing(int percent);
    Q_INVOKABLE void setViewSize(int width, int height);

    Q_INVOKABLE void relayout();

    int renderVersion() const;

    Q_INVOKABLE void bumpRenderVersion();

    Q_INVOKABLE void emitCurrentPageChanged();

    Q_INVOKABLE QStringList availableFontFaces() const;

    bool isLandscape() const;

    Q_INVOKABLE void toggleOrientation();

    LVDocView* docView() const { return m_docView; }

signals:
    void pageCountChanged();
    void currentPageChanged();
    void loadedChanged();
    void documentLoadProgress(int percent);

    void orientationChanged();

    void renderVersionChanged();

private:

    void initFontManager();

    int contentWidth() const;
    int contentHeight() const;

    LVDocView *m_docView   = nullptr;
    bool       m_loaded    = false;
    QString    m_documentPath;
    bool       m_fontManagerInitialized = false;
    int        m_width     = 954;
    int        m_height    = 1696;
    bool       m_isLandscape = false;
    int        m_renderVersion = 0;

    int        m_marginLeft   = 48;
    int        m_marginTop    = 64;
    int        m_marginRight  = 48;
    int        m_marginBottom = 64;
};
