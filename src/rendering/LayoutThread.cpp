#include "LayoutThread.h"

#include "lvdocview.h"
#include "lvdrawbuf.h"
#include "lvfntman.h"
#include "lvrend.h"

#include <QDebug>
#include <QMutexLocker>

LayoutThread::LayoutThread(PageCache *cache, QObject *parent)
    : QThread(parent)
    , m_cache(cache)
{
    Q_ASSERT(cache);
}

LayoutThread::~LayoutThread()
{
    stopAndWait();
}

void LayoutThread::setDocument(const QString &epubPath)
{
    QMutexLocker lock(&m_requestMutex);
    m_epubPath = epubPath;
    m_docLoaded = false;

    lock.unlock();
    m_cache->invalidateAll();
}

void LayoutThread::setCurrentPage(int page)
{
    QMutexLocker lock(&m_requestMutex);
    m_currentPage = page;
}

void LayoutThread::setViewSize(int width, int height)
{
    QMutexLocker lock(&m_requestMutex);
    m_pendingWidth  = width;
    m_pendingHeight = height;

    m_settings.viewWidth  = width;
    m_settings.viewHeight = height;
}

void LayoutThread::applySettings(const ReadingSettings &settings)
{
    {
        QMutexLocker lock(&m_requestMutex);
        m_settings = settings;
    }

    m_requestCondition.wakeOne();
}

void LayoutThread::requestPrerender(int centerPage)
{
    {
        QMutexLocker lock(&m_requestMutex);

        m_pendingRequests.clear();
        m_pendingRequests.enqueue(centerPage);
    }
    m_requestCondition.wakeOne();
}

void LayoutThread::stopAndWait()
{
    if (!isRunning())
        return;

    m_stop.store(true, std::memory_order_relaxed);
    m_requestCondition.wakeAll();
    wait();
}

void LayoutThread::run()
{

    while (!m_stop.load(std::memory_order_relaxed)) {
        int centerPage = -1;
        QString epubPath;
        ReadingSettings settings;

        {
            QMutexLocker lock(&m_requestMutex);

            while (!m_stop.load(std::memory_order_relaxed)
                   && m_pendingRequests.isEmpty()
                   && m_settings == m_appliedSettings
                   && m_docLoaded) {
                m_requestCondition.wait(&m_requestMutex);
            }

            if (m_stop.load(std::memory_order_relaxed))
                break;

            epubPath = m_epubPath;
            settings = m_settings;
            if (!m_pendingRequests.isEmpty())
                centerPage = m_pendingRequests.dequeue();
            else
                centerPage = m_currentPage;
        }

        if (!m_docLoaded && !epubPath.isEmpty()) {

            delete m_docView;
            m_docView = nullptr;

            m_docView = new LVDocView(8);
            m_docView->setViewMode(DVM_PAGES, 1);
            m_docView->setPageHeaderInfo(PGHDR_NONE);

            const std::string pathStd = epubPath.toStdString();
            if (!m_docView->LoadDocument(pathStd.c_str())) {
                qWarning() << "LayoutThread: LoadDocument failed for" << epubPath;
                delete m_docView;
                m_docView = nullptr;
                continue;
            }

            m_docLoaded = true;

            m_appliedSettings = ReadingSettings{};
        }

        if (!m_docLoaded || !m_docView)
            continue;

        if (settings != m_appliedSettings) {
            applySettingsToView();
            m_cache->invalidateAll();
        }

        if (centerPage < 1)
            continue;

        const int total = m_docView->getPageCount();
        if (total <= 0)
            continue;

        const int order[] = { 1, 2, 0, -1, -2 };
        for (int delta : order) {
            if (m_stop.load(std::memory_order_relaxed))
                break;
            const int page = centerPage + delta;
            if (page < 1 || page > total)
                continue;
            if (m_cache->contains(page))
                continue;
            renderOnePage(page);
        }

        emit prerenderComplete();
    }

    delete m_docView;
    m_docView = nullptr;
}

void LayoutThread::applySettingsToView()
{
    if (!m_docView)
        return;

    ReadingSettings s;
    {
        QMutexLocker lock(&m_requestMutex);
        s = m_settings;
    }

    m_docView->Resize(s.viewWidth, s.viewHeight);

    const std::string fontName = s.fontFace.toStdString();
    m_docView->setDefaultFontFace(lString8(fontName.c_str()));
    m_docView->setFontSize(s.fontSize);

    lvRect margins;
    margins.left   = s.marginLeft;
    margins.top    = s.marginTop;
    margins.right  = s.marginRight;
    margins.bottom = s.marginBottom;
    m_docView->setPageMargins(margins);

    m_docView->setDefaultInterlineSpace(s.lineSpacing);

    m_appliedSettings = s;
}

bool LayoutThread::renderOnePage(int pageNumber)
{
    if (!m_docView)
        return false;

    const int internalPage = pageNumber - 1;
    const int total = m_docView->getPageCount();

    if (internalPage < 0 || internalPage >= total)
        return false;

    if (!m_docView->goToPage(internalPage)) {
        qWarning() << "LayoutThread: goToPage failed for" << pageNumber;
        return false;
    }

    const int w = m_appliedSettings.viewWidth;
    const int h = m_appliedSettings.viewHeight;

    LVGrayDrawBuf drawBuf(w, h, 8);
    drawBuf.Clear(0xFF);

    m_docView->Draw(drawBuf, true);

    const lUInt8 *bits     = drawBuf.GetScanLine(0);
    const int     rowBytes = drawBuf.GetRowSize();

    QImage img(bits, w, h, rowBytes, QImage::Format_Grayscale8);
    QImage page = img.copy();

    m_cache->insert(pageNumber, page);
    emit pageReady(pageNumber);
    return true;
}

void LayoutThread::renderRange(int fromPage, int toPage)
{
    for (int p = fromPage; p <= toPage; ++p) {
        if (m_stop.load(std::memory_order_relaxed))
            return;
        if (!m_cache->contains(p))
            renderOnePage(p);
    }
}
