#include "CrEngineRenderer.h"

#include "lvdocview.h"
#include "lvdrawbuf.h"
#include "lvfntman.h"
#include "lvrend.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

CrEngineRenderer::CrEngineRenderer(QObject *parent)
    : QObject(parent)
{

    QSettings settings(QStringLiteral("ReadMarkable"), QStringLiteral("reader"));
    m_isLandscape = settings.value(QStringLiteral("orientation/isLandscape"), false).toBool();
    if (m_isLandscape) {
        m_width  = 1696;
        m_height = 954;
    }

    qInfo() << "CrEngineRenderer: initialized (width=" << m_width
            << "height=" << m_height
            << "landscape=" << m_isLandscape << ")";
}

CrEngineRenderer::~CrEngineRenderer()
{
    closeDocument();
}

void CrEngineRenderer::initFontManager()
{
    if (m_fontManagerInitialized)
        return;

    if (!InitFontManager(lString8(""))) {
        qWarning() << "CrEngineRenderer: InitFontManager() returned false";

    }

    gRenderDPI = 264;

    const QStringList fontDirs = {
        QStringLiteral("/home/root/.readmarkable/fonts"),
        QStringLiteral("/usr/share/fonts/ttf/noto"),
        QStringLiteral("/usr/share/fonts/ttf/ebgaramond"),
    };
    const QStringList filters = { QStringLiteral("*.ttf"), QStringLiteral("*.otf") };
    int totalRegistered = 0;
    for (const QString &fontDir : fontDirs) {
        if (!QDir(fontDir).exists())
            continue;
        const QFileInfoList fontFiles = QDir(fontDir).entryInfoList(filters, QDir::Files);
        for (const QFileInfo &fi : fontFiles) {
            const std::string path = fi.absoluteFilePath().toStdString();
            if (fontMan && fontMan->RegisterFont(lString8(path.c_str()))) {
                totalRegistered++;
            }
        }
    }
    qInfo() << "CrEngineRenderer: registered" << totalRegistered << "fonts";

    m_fontManagerInitialized = true;
}

bool CrEngineRenderer::loadDocument(const QString &epubPath)
{
    if (!QFileInfo::exists(epubPath)) {
        qWarning() << "CrEngineRenderer: file not found:" << epubPath;
        return false;
    }

    m_documentPath = QFileInfo(epubPath).absoluteFilePath();

    initFontManager();

    closeDocument();

    m_docView = new LVDocView(8);

    m_docView->setViewMode(DVM_PAGES, 1);

    m_docView->Resize(m_width, m_height);

    m_docView->setDefaultFontFace(lString8("Noto Serif"));
    m_docView->setFontSize(14);

    lvRect margins;
    margins.left   = 0;
    margins.right  = 0;
    margins.top    = 0;
    margins.bottom = 0;
    m_docView->setPageMargins(margins);

    m_docView->setDefaultInterlineSpace(100);

    m_docView->setPageHeaderInfo(PGHDR_NONE);

    const std::string pathStd = epubPath.toStdString();
    if (!m_docView->LoadDocument(pathStd.c_str())) {
        qWarning() << "CrEngineRenderer: LVDocView::LoadDocument() failed for" << epubPath;
        delete m_docView;
        m_docView = nullptr;
        return false;
    }

    m_docView->Render(m_width, m_height);

    m_loaded = true;
    emit loadedChanged();
    emit pageCountChanged();
    emit currentPageChanged();

    qInfo() << "CrEngineRenderer: loaded document" << epubPath
            << "pages:" << pageCount();
    return true;
}

void CrEngineRenderer::closeDocument()
{
    if (!m_docView)
        return;

    delete m_docView;
    m_docView = nullptr;

    m_documentPath.clear();

    const bool wasLoaded = m_loaded;
    m_loaded = false;

    if (wasLoaded) {
        emit loadedChanged();
        emit pageCountChanged();
        emit currentPageChanged();
    }
}

QString CrEngineRenderer::documentPath() const
{
    return m_documentPath;
}

bool CrEngineRenderer::isLoaded() const
{
    return m_loaded && m_docView != nullptr;
}

QImage CrEngineRenderer::renderPage(int pageNumber)
{
    if (!isLoaded())
        return {};

    const int internalPage = pageNumber - 1;
    if (internalPage < 0 || internalPage >= m_docView->getPageCount()) {
        qWarning() << "CrEngineRenderer::renderPage: out of range" << pageNumber
                   << "(count=" << pageCount() << ")";
        return {};
    }

    if (!m_docView->goToPage(internalPage)) {
        qWarning() << "CrEngineRenderer::renderPage: goToPage failed for page" << pageNumber;
        return {};
    }

    LVGrayDrawBuf drawBuf(m_width, m_height, 8);
    drawBuf.Clear(0xFF);
    m_docView->Draw(drawBuf, true);

    const lUInt8 *bits = drawBuf.GetScanLine(0);
    const int rowBytes = drawBuf.GetRowSize();
    QImage fullPage(bits, m_width, m_height, rowBytes, QImage::Format_Grayscale8);
    QImage result = fullPage.copy();

    return result;
}

QImage CrEngineRenderer::renderCurrentPage()
{
    if (!isLoaded())
        return {};
    return renderPage(currentPage());
}

int CrEngineRenderer::pageCount() const
{
    if (!isLoaded())
        return 0;
    return m_docView->getPageCount();
}

int CrEngineRenderer::currentPage() const
{
    if (!isLoaded())
        return 0;

    return m_docView->getCurPage() + 1;
}

void CrEngineRenderer::goToPage(int page)
{
    if (!isLoaded())
        return;

    const int internalPage = page - 1;
    if (internalPage < 0 || internalPage >= m_docView->getPageCount())
        return;

    if (m_docView->goToPage(internalPage))
        emit currentPageChanged();
}

QString CrEngineRenderer::currentXPointer() const
{
    if (!isLoaded())
        return {};

    ldomXPointer bm = m_docView->getBookmark();
    if (bm.isNull())
        return {};

    lString32 xpStr = bm.toString();
    return QString::fromUtf8(UnicodeToUtf8(xpStr).c_str());
}

void CrEngineRenderer::goToXPointer(const QString &xpointer)
{
    if (!isLoaded() || xpointer.isEmpty())
        return;

    ldomDocument *dom = m_docView->getDocument();
    if (!dom)
        return;

    const std::string utf8 = xpointer.toStdString();
    lString32 xpStr = Utf8ToUnicode(utf8.c_str());
    ldomXPointer bm = dom->createXPointer(xpStr);
    if (!bm.isNull()) {
        m_docView->goToBookmark(bm);
        emit currentPageChanged();
    }
}

void CrEngineRenderer::setFontFace(const QString &fontName)
{
    if (!isLoaded())
        return;
    const std::string name = fontName.toStdString();
    m_docView->setDefaultFontFace(lString8(name.c_str()));
}

void CrEngineRenderer::setFontSize(int ptSize)
{
    if (!isLoaded())
        return;
    m_docView->setFontSize(ptSize);
}

void CrEngineRenderer::setPageMargins(int left, int top, int right, int bottom)
{
    Q_UNUSED(left); Q_UNUSED(top); Q_UNUSED(right); Q_UNUSED(bottom);

}

int CrEngineRenderer::contentWidth() const
{
    return m_width - m_marginLeft - m_marginRight;
}

int CrEngineRenderer::contentHeight() const
{
    return m_height - m_marginTop - m_marginBottom;
}

void CrEngineRenderer::setLineSpacing(int percent)
{
    if (!isLoaded())
        return;
    m_docView->setDefaultInterlineSpace(percent);
}

void CrEngineRenderer::setViewSize(int width, int height)
{
    m_width  = width;
    m_height = height;
    if (isLoaded())
        m_docView->Resize(m_width, m_height);
}

void CrEngineRenderer::relayout()
{
    if (!isLoaded())
        return;

    m_docView->Render(m_width, m_height);
    ++m_renderVersion;
    emit pageCountChanged();
    emit currentPageChanged();
    emit renderVersionChanged();
}

int CrEngineRenderer::renderVersion() const
{
    return m_renderVersion;
}

void CrEngineRenderer::emitCurrentPageChanged()
{
    emit currentPageChanged();
}

void CrEngineRenderer::bumpRenderVersion()
{
    ++m_renderVersion;
    emit renderVersionChanged();
}

QStringList CrEngineRenderer::availableFontFaces() const
{

    if (!fontMan)
        return {};

    lString32Collection faces;
    fontMan->getFaceList(faces);

    QStringList result;
    result.reserve(static_cast<int>(faces.length()));
    for (int i = 0; i < static_cast<int>(faces.length()); ++i) {
        lString32 face = faces[i];
        result.append(QString::fromUtf8(UnicodeToUtf8(face).c_str()));
    }
    return result;
}

bool CrEngineRenderer::isLandscape() const
{
    return m_isLandscape;
}

void CrEngineRenderer::toggleOrientation()
{

    const QString savedXPointer = currentXPointer();

    m_isLandscape = !m_isLandscape;
    std::swap(m_width, m_height);

    qInfo() << "CrEngineRenderer: orientation toggled to"
            << (m_isLandscape ? "landscape" : "portrait")
            << "(" << m_width << "x" << m_height << ")";

    setViewSize(m_width, m_height);
    relayout();

    if (!savedXPointer.isEmpty())
        goToXPointer(savedXPointer);

    emit orientationChanged();

    emit pageCountChanged();

    QSettings settings(QStringLiteral("ReadMarkable"), QStringLiteral("reader"));
    settings.setValue(QStringLiteral("orientation/isLandscape"), m_isLandscape);
    settings.sync();
}
