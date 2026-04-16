#pragma once

#include "PageCache.h"
#include "ReadingSettings.h"

#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>

#include <atomic>

class LVDocView;

class LayoutThread : public QThread
{
    Q_OBJECT

public:
    explicit LayoutThread(PageCache *cache, QObject *parent = nullptr);
    ~LayoutThread() override;

    void setDocument(const QString &epubPath);

    void setCurrentPage(int page);

    void setViewSize(int width, int height);

    void applySettings(const ReadingSettings &settings);

    void requestPrerender(int centerPage);

    void stopAndWait();

signals:

    void pageReady(int pageNumber);

    void prerenderComplete();

protected:
    void run() override;

private:

    void applySettingsToView();

    void renderRange(int fromPage, int toPage);

    bool renderOnePage(int pageNumber);

    PageCache       *m_cache;
    QString          m_epubPath;
    int              m_currentPage  = 1;
    ReadingSettings  m_settings;
    int              m_pendingWidth  = 954;
    int              m_pendingHeight = 1696;

    QMutex           m_requestMutex;
    QWaitCondition   m_requestCondition;
    QQueue<int>      m_pendingRequests;
    std::atomic<bool> m_stop{false};

    LVDocView       *m_docView       = nullptr;
    ReadingSettings  m_appliedSettings;
    bool             m_docLoaded     = false;
};
