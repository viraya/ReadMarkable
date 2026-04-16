#include "SearchService.h"
#include "SearchDatabase.h"

#include "../epub/CrEngineRenderer.h"
#include "../navigation/TocModel.h"

#include "lvdocview.h"
#include "lvstring.h"

#include <QDebug>
#include <QDir>
#include <QRect>
#include <QVariantMap>

#include "lvtinydom.h"

SearchService::SearchService(QObject *parent)
    : QObject(parent)
{

    const QString dataDir = QStringLiteral("/home/root/.readmarkable/data");
    QDir().mkpath(dataDir);

    const QString dbPath = dataDir + QStringLiteral("/search.db");
    m_db = std::make_unique<SearchDatabase>(dbPath);

    if (!m_db->isOpen()) {
        qWarning() << "SearchService: database failed to open at" << dbPath;
    } else {
        qDebug() << "SearchService: initialized with database at" << dbPath;
    }
}

SearchService::~SearchService() = default;

QVariantList SearchService::results() const
{
    QVariantList list;
    list.reserve(m_results.size());

    for (const SearchResult &r : m_results) {
        QVariantMap map;
        map[QStringLiteral("pageNum")]      = r.pageNum;
        map[QStringLiteral("chapterTitle")] = r.chapterTitle;
        map[QStringLiteral("snippet")]      = r.snippet;
        list.append(map);
    }

    return list;
}

void SearchService::setCurrentBook(const QString &bookPath)
{
    if (bookPath.isEmpty()) {
        m_currentBookPath.clear();
        m_currentBookSha.clear();
        m_indexReady = false;
        emit indexReadyChanged();
        return;
    }

    m_currentBookPath = bookPath;
    m_currentBookSha  = SearchDatabase::bookSha(bookPath);

    const bool hasIdx = m_db && m_db->hasIndex(m_currentBookSha);
    if (hasIdx != m_indexReady) {
        m_indexReady = hasIdx;
        emit indexReadyChanged();
    }

    qDebug() << "SearchService: current book set, sha ="
             << m_currentBookSha.left(12) << "... indexReady =" << m_indexReady;
}

void SearchService::buildIndex(CrEngineRenderer *renderer, TocModel *tocModel)
{
    if (!renderer || !tocModel) {
        qWarning() << "SearchService::buildIndex: null renderer or tocModel";
        return;
    }
    if (!renderer->isLoaded()) {
        qWarning() << "SearchService::buildIndex: renderer not loaded";
        return;
    }
    if (!m_db || !m_db->isOpen()) {
        qWarning() << "SearchService::buildIndex: database not open";
        return;
    }
    if (m_currentBookSha.isEmpty()) {
        qWarning() << "SearchService::buildIndex: no current book set";
        return;
    }

    LVDocView *docView = renderer->docView();
    if (!docView) {
        qWarning() << "SearchService::buildIndex: docView is null";
        return;
    }

    qDebug() << "SearchService: building FTS5 index for"
             << m_currentBookPath;

    m_db->clearBook(m_currentBookSha);

    const int totalPages = docView->getPageCount();
    if (totalPages <= 0) {
        qWarning() << "SearchService::buildIndex: document has 0 pages";
        return;
    }

    qDebug() << "SearchService: indexing" << totalPages << "pages...";

    for (int p = 0; p < totalPages; ++p) {

        const lString32 pageText = docView->getPageText(false, p);
        const QString bodyText = QString::fromUtf8(UnicodeToUtf8(pageText).c_str());

        QString chapterTitle;
        const int chapterIdx = tocModel->findChapterForPage(p);
        if (chapterIdx >= 0) {
            const QModelIndex modelIdx = tocModel->index(chapterIdx);
            chapterTitle = tocModel->data(modelIdx, TocModel::TitleRole).toString();
        }

        m_db->insertPage(m_currentBookSha, p + 1, chapterTitle, bodyText);

        if (p < 3 || p == totalPages - 1) {
            const QString preview = bodyText.left(60).simplified();
            qDebug() << "  page" << (p + 1) << ":" << preview << "...";
        }
    }

    m_db->rebuildFtsIndex();

    m_indexReady = true;
    emit indexReadyChanged();

    qDebug() << "SearchService: index complete for"
             << totalPages << "pages";
}

void SearchService::search(const QString &query)
{
    const QString trimmed = query.trimmed();

    if (trimmed.length() < 2) {
        clearResults();
        return;
    }
    if (!m_db || !m_db->isOpen() || m_currentBookSha.isEmpty()) {
        clearResults();
        return;
    }

    m_results = m_db->search(m_currentBookSha, trimmed);
    emit resultsChanged();

    qDebug() << "SearchService: search for" << query
             << "returned" << m_results.size() << "results";
    for (int i = 0; i < qMin(m_results.size(), 5); ++i) {
        qDebug() << "  result" << i << ": page" << m_results[i].pageNum
                 << "chapter:" << m_results[i].chapterTitle.left(30)
                 << "snippet:" << m_results[i].snippet.left(60);
    }
}

void SearchService::clearResults()
{
    if (!m_results.isEmpty()) {
        m_results.clear();
        emit resultsChanged();
    }
}

int SearchService::pageForResultAt(int index) const
{
    if (index < 0 || index >= m_results.size())
        return 0;
    return m_results.at(index).pageNum;
}

QVariantList SearchService::findTextRects(const QString &query,
                                          CrEngineRenderer *renderer) const
{
    QVariantList result;
    if (!renderer || !renderer->isLoaded() || query.isEmpty()) {
        qDebug() << "findTextRects: early return  -  renderer/query invalid";
        return result;
    }

    LVDocView *dv = renderer->docView();
    if (!dv) {
        qDebug() << "findTextRects: early return  -  docView null";
        return result;
    }

    qDebug() << "findTextRects: searching for" << query
             << "on page" << (dv->getCurPage() + 1);

    const lString32 pattern = Utf8ToUnicode(query.toUtf8().constData());

    const int curPage = dv->getCurPage();
    const int docY = dv->GetPos();
    const int docH = dv->GetHeight();

    LVArray<ldomWord> words;
    dv->getDocument()->findText(
        pattern,
        true,
        false,
        docY,
        docY + docH,
        words,
        20,
        100000);

    qDebug() << "findTextRects: findText on page" << (curPage + 1)
             << "docY:" << docY << "-" << (docY + docH)
             << "returned" << words.length() << "words";

    if (words.empty())
        return result;

    for (int w = 0; w < words.length(); ++w) {
        ldomXRange range(words[w]);
        LVArray<lvRect> docRects;
        range.getSegmentRects(docRects);

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
    }

    qDebug() << "SearchService::findTextRects: found" << result.size()
             << "rects for" << query << "on page" << (curPage + 1);
    return result;
}
