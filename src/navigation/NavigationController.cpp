#include "NavigationController.h"
#include "TocModel.h"
#include "BookmarkModel.h"
#include "BookmarkRepository.h"
#include "ReadingTimeEstimator.h"
#include "../epub/CrEngineRenderer.h"
#include "../epub/EpubParser.h"
#include "../epub/EpubDocument.h"

#include "lvdocview.h"
#include "lvtinydom.h"

#include <QDebug>
#include <QFileInfo>
#include <QStringList>

NavigationController::NavigationController(CrEngineRenderer *renderer,
                                           BookmarkRepository *bookmarkRepo,
                                           QObject *parent)
    : QObject(parent)
    , m_renderer(renderer)
    , m_bookmarkRepo(bookmarkRepo)
    , m_tocModel(new TocModel(this))
    , m_bookmarkModel(new BookmarkModel(this))
    , m_readingTimeEstimator(new ReadingTimeEstimator(this))
{
    Q_ASSERT(renderer);
    Q_ASSERT(bookmarkRepo);

    connect(m_renderer, &CrEngineRenderer::currentPageChanged,
            this, &NavigationController::refreshProgress);
    connect(m_renderer, &CrEngineRenderer::currentPageChanged,
            this, &NavigationController::refreshBookmarkState);

    connect(m_renderer, &CrEngineRenderer::loadedChanged, this, [this]() {
        if (!m_renderer->isLoaded()) {
            m_tocModel->clear();
            m_bookmarkModel->clear();
            m_readingTimeEstimator->reset();
            m_progressPercent            = 0;
            m_currentChapterIndex        = -1;
            m_totalChapters              = 0;
            m_currentChapterTitle.clear();
            m_canGoBack                  = false;
            m_currentPageBookmarked      = false;
            m_minutesRemainingInChapter  = 0;
            const bool hadTitle = !m_currentBookTitle.isEmpty();
            m_currentBookTitle.clear();
            emit progressChanged();
            emit canGoBackChanged();
            emit bookmarkStateChanged();
            if (hadTitle)
                emit currentBookTitleChanged();
        }
    });

    qDebug() << "NavigationController: initialized";
}

NavigationController::~NavigationController() = default;

LVDocView *NavigationController::docView() const
{
    if (!m_renderer || !m_renderer->isLoaded())
        return nullptr;
    return m_renderer->docView();
}

int NavigationController::progressPercent() const
{
    return m_progressPercent;
}

int NavigationController::chapterProgressPercent() const
{

    if (!m_renderer || !m_renderer->isLoaded())
        return 0;
    if (!m_tocModel || m_tocModel->count() == 0)
        return 0;

    const int page0 = m_renderer->currentPage() - 1;
    if (page0 < 0)
        return 0;

    const int chapterIdx = m_tocModel->findChapterForPage(page0);
    if (chapterIdx < 0)
        return 0;

    const QModelIndex curMi = m_tocModel->index(chapterIdx, 0);
    const int chapterStart = m_tocModel->data(curMi, TocModel::PageRole).toInt() - 1;

    int chapterEnd = m_renderer->pageCount();
    for (int i = chapterIdx + 1; i < m_tocModel->count(); ++i) {
        const QModelIndex mi = m_tocModel->index(i, 0);
        const int p = m_tocModel->data(mi, TocModel::PageRole).toInt() - 1;
        if (p > chapterStart) {
            chapterEnd = p;
            break;
        }
    }

    const int span = chapterEnd - chapterStart;
    if (span <= 0)
        return 0;

    const int within = page0 - chapterStart;

    int pct = (within * 100 + span / 2) / span;
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

int NavigationController::currentChapterIndex() const
{
    return m_currentChapterIndex;
}

int NavigationController::totalChapters() const
{
    return m_totalChapters;
}

QString NavigationController::currentChapterTitle() const
{
    return m_currentChapterTitle;
}

bool NavigationController::canGoBack() const
{
    return m_canGoBack;
}

bool NavigationController::currentPageBookmarked() const
{
    return m_currentPageBookmarked;
}

int NavigationController::minutesRemainingInChapter() const
{
    return m_minutesRemainingInChapter;
}

TocModel *NavigationController::tocModel() const
{
    return m_tocModel;
}

BookmarkModel *NavigationController::bookmarkModel() const
{
    return m_bookmarkModel;
}

void NavigationController::nextPage()
{
    if (!m_renderer->isLoaded())
        return;
    const int next = m_renderer->currentPage() + 1;
    m_renderer->goToPage(next);

}

void NavigationController::previousPage()
{
    if (!m_renderer->isLoaded())
        return;
    const int prev = m_renderer->currentPage() - 1;
    m_renderer->goToPage(prev);
}

void NavigationController::nextChapter()
{
    LVDocView *dv = docView();
    if (!dv)
        return;
    dv->moveByChapter(1);

    m_renderer->bumpRenderVersion();
    m_renderer->emitCurrentPageChanged();
    refreshProgress();
    refreshBookmarkState();
}

void NavigationController::previousChapter()
{
    LVDocView *dv = docView();
    if (!dv)
        return;
    dv->moveByChapter(-1);
    m_renderer->bumpRenderVersion();
    m_renderer->emitCurrentPageChanged();
    refreshProgress();
    refreshBookmarkState();
}

void NavigationController::goToTocEntry(int index)
{
    LVDocView *dv = docView();
    if (!dv)
        return;

    const QString xpointer = m_tocModel->xpointerAt(index);
    if (xpointer.isEmpty()) {
        qWarning() << "NavigationController::goToTocEntry: no xpointer at index" << index;
        return;
    }

    m_renderer->goToXPointer(xpointer);

}

void NavigationController::goToBookmarkXPointer(const QString &xpointer)
{
    if (xpointer.isEmpty())
        return;

    m_renderer->goToXPointer(xpointer);

}

void NavigationController::goBack()
{
    LVDocView *dv = docView();
    if (!dv)
        return;
    if (dv->goBack()) {
        m_renderer->bumpRenderVersion();
        refreshProgress();
        refreshBookmarkState();
    }
}

void NavigationController::toggleBookmark()
{
    LVDocView *dv = docView();
    if (!dv || m_bookPath.isEmpty())
        return;

    const QString xpointer = m_renderer->currentXPointer();
    if (xpointer.isEmpty())
        return;

    const int percent = dv->getPosPercent();

    if (m_currentPageBookmarked) {

        m_bookmarkRepo->remove(m_bookPath, xpointer);
        qDebug() << "NavigationController: removed bookmark at" << xpointer;
    } else {

        m_bookmarkRepo->save(m_bookPath, xpointer, percent);
        qDebug() << "NavigationController: added bookmark at" << xpointer;
    }

    refreshBookmarkState();

    m_bookmarkModel->refresh(dv, m_bookmarkRepo, m_bookPath);
}

QString NavigationController::checkLink(int contentX, int contentY)
{
    LVDocView *dv = docView();
    if (!dv)
        return {};

    lvPoint pt(contentX, contentY);
    if (!dv->windowToDocPoint(pt, false))
        return {};

    ldomXPointer node = dv->getNodeByPoint(pt, false, false);
    if (node.isNull())
        return {};

    ldomNode *elem = node.getNode();
    while (elem && !elem->isNull()) {
        if (elem->isElement()) {

            const lString32 href = elem->getAttributeValue(U"href");
            if (!href.empty()) {
                const QString hrefStr = QString::fromUtf8(
                    UnicodeToUtf8(href).c_str());
                const bool isFootnote = hrefStr.startsWith(QLatin1Char('#'));
                emit internalLinkDetected(hrefStr, isFootnote);
                return hrefStr;
            }
        }
        elem = elem->getParentNode();
    }

    return {};
}

QString NavigationController::getLinkTargetText(const QString &href)
{
    LVDocView *dv = docView();
    if (!dv || href.isEmpty())
        return {};

    if (!href.startsWith(QLatin1Char('#')))
        return {};

    ldomDocument *dom = dv->getDocument();
    if (!dom)
        return {};

    const QString id = href.mid(1);
    if (id.isEmpty())
        return {};

    const std::string idUtf8 = id.toStdString();
    const lString32 idL32 = Utf8ToUnicode(idUtf8.c_str());

    ldomNode *target = dom->getElementById(idL32.c_str());

    if (!target || target->isNull())
        return {};

    const lString32 text = target->getText(U'\n');
    if (text.empty())
        return {};

    return QString::fromUtf8(UnicodeToUtf8(text).c_str());
}

void NavigationController::navigateLink(const QString &href)
{
    LVDocView *dv = docView();
    if (!dv || href.isEmpty())
        return;

    const std::string hrefUtf8 = href.toStdString();
    const lString32 hrefLStr = Utf8ToUnicode(hrefUtf8.c_str());

    if (dv->goLink(hrefLStr, true)) {
        m_renderer->bumpRenderVersion();
        refreshProgress();
        refreshBookmarkState();
        emit linkNavigated();
    }
}

void NavigationController::refreshProgress()
{
    LVDocView *dv = docView();
    if (!dv) {
        m_progressPercent     = 0;
        m_currentChapterIndex = -1;
        m_totalChapters       = 0;
        m_currentChapterTitle.clear();
        m_canGoBack           = false;
        emit progressChanged();
        emit canGoBackChanged();
        return;
    }

    const int rawPercent    = dv->getPosPercent();
    const int newPercent    = rawPercent / 100;

    const int page0 = m_renderer->currentPage() - 1;

    const int chapterIdx    = m_tocModel->findChapterForPage(page0);
    const int totalChaps    = m_tocModel->count();

    QString chapterTitle;
    if (chapterIdx >= 0) {
        const QModelIndex mi = m_tocModel->index(chapterIdx, 0);
        chapterTitle = m_tocModel->data(mi, TocModel::TitleRole).toString();
    }

    const bool backAvail = (dv->getNavigationHistory().backCount() > 0);

    const int minsRemaining = m_readingTimeEstimator->minutesRemainingInChapter(page0);

    const bool progressDirty =
        (newPercent    != m_progressPercent)  ||
        (chapterIdx    != m_currentChapterIndex) ||
        (totalChaps    != m_totalChapters)    ||
        (chapterTitle  != m_currentChapterTitle) ||
        (minsRemaining != m_minutesRemainingInChapter);

    const bool backDirty = (backAvail != m_canGoBack);

    m_progressPercent            = newPercent;
    m_currentChapterIndex        = chapterIdx;
    m_totalChapters              = totalChaps;
    m_currentChapterTitle        = chapterTitle;
    m_canGoBack                  = backAvail;
    m_minutesRemainingInChapter  = minsRemaining;

    if (progressDirty)
        emit progressChanged();
    if (backDirty)
        emit canGoBackChanged();
}

void NavigationController::refreshBookmarkState()
{
    LVDocView *dv = docView();
    if (!dv || m_bookPath.isEmpty()) {
        if (m_currentPageBookmarked) {
            m_currentPageBookmarked = false;
            emit bookmarkStateChanged();
        }
        return;
    }

    const QString xpointer = m_renderer->currentXPointer();
    const bool bookmarked = !xpointer.isEmpty() &&
                            m_bookmarkRepo->hasBookmarkAt(m_bookPath, xpointer);

    if (bookmarked != m_currentPageBookmarked) {
        m_currentPageBookmarked = bookmarked;
        emit bookmarkStateChanged();
    }
}

void NavigationController::onDocumentLoaded(const QString &bookPath)
{
    m_bookPath = bookPath;

    LVDocView *dv = docView();
    if (!dv)
        return;

    m_tocModel->populate(dv);
    qDebug() << "NavigationController: TOC loaded with" << m_tocModel->count() << "entries";

    QString newTitle;
    const lString32 crTitle = dv->getTitle();
    if (!crTitle.empty())
        newTitle = QString::fromUtf8(UnicodeToUtf8(crTitle).c_str()).trimmed();

    if (newTitle.isEmpty()) {
        const EpubDocument parsed = EpubParser::parse(bookPath);
        if (!parsed.title.isEmpty())
            newTitle = parsed.title.trimmed();
    }

    if (newTitle.isEmpty())
        newTitle = deriveBookTitleFromPath(bookPath);

    if (newTitle != m_currentBookTitle) {
        m_currentBookTitle = newTitle;
        emit currentBookTitleChanged();
    }

    m_readingTimeEstimator->computeChapterWordCounts(dv);

    m_bookmarkModel->refresh(dv, m_bookmarkRepo, m_bookPath);

    refreshProgress();
    refreshBookmarkState();
}

QString NavigationController::deriveBookTitleFromPath(const QString &path) const
{
    if (path.isEmpty())
        return {};

    QFileInfo fi(path);
    QString base = fi.completeBaseName();
    if (base.isEmpty())
        base = fi.fileName();

    base.replace(QLatin1Char('_'), QLatin1Char(' '));
    base.replace(QLatin1Char('-'), QLatin1Char(' '));

    const QStringList words = base.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList titled;
    titled.reserve(words.size());
    for (const QString &w : words) {
        if (w.isEmpty())
            continue;
        titled << (w.at(0).toUpper() + w.mid(1));
    }
    return titled.join(QLatin1Char(' '));
}
