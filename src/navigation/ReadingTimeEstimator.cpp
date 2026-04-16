#include "ReadingTimeEstimator.h"

#include "lvdocview.h"
#include "lvtinydom.h"

#include <QDebug>
#include <QRegularExpression>
#include <cmath>

ReadingTimeEstimator::ReadingTimeEstimator(QObject *parent)
    : QObject(parent)
{
}

void ReadingTimeEstimator::computeChapterWordCounts(LVDocView *docView)
{
    Q_ASSERT(docView);
    reset();

    const int totalPages = docView->getPageCount();
    if (totalPages <= 0) {
        qWarning() << "ReadingTimeEstimator: document has 0 pages";
        return;
    }

    m_totalPages = totalPages;
    m_pageChapter.fill(-1, totalPages);
    m_pageWords.fill(0, totalPages);

    LVPtrVector<LVTocItem, false> items;
    const bool hasToc = docView->getFlatToc(items);

    QVector<int> chapterStartPages;

    if (hasToc) {
        for (int i = 0; i < items.length(); ++i) {
            LVTocItem *item = items[i];
            if (!item)
                continue;

            if (item->getLevel() != 1)
                continue;
            const int page0 = item->getPage();
            if (page0 >= 0 && page0 < totalPages)
                chapterStartPages.append(page0);
        }
    }

    if (chapterStartPages.isEmpty()) {

        ChapterInfo single;
        single.firstPage0 = 0;
        single.lastPage0  = totalPages - 1;
        single.wordCount  = 0;
        m_chapters.append(single);
    } else {

        for (int ci = 0; ci < chapterStartPages.size(); ++ci) {
            ChapterInfo ch;
            ch.firstPage0 = chapterStartPages[ci];
            ch.lastPage0  = (ci + 1 < chapterStartPages.size())
                            ? chapterStartPages[ci + 1] - 1
                            : totalPages - 1;
            ch.wordCount  = 0;
            m_chapters.append(ch);
        }

        if (chapterStartPages.first() > 0) {
            ChapterInfo preamble;
            preamble.firstPage0 = 0;
            preamble.lastPage0  = chapterStartPages.first() - 1;
            preamble.wordCount  = 0;
            m_chapters.prepend(preamble);
        }
    }

    for (int ci = 0; ci < m_chapters.size(); ++ci) {
        const ChapterInfo &ch = m_chapters[ci];
        for (int p = ch.firstPage0; p <= ch.lastPage0; ++p) {
            if (p >= 0 && p < totalPages)
                m_pageChapter[p] = ci;
        }
    }

    int totalWords = 0;
    for (int p = 0; p < totalPages; ++p) {
        const lString32 pageText = docView->getPageText(false, p);
        if (pageText.empty()) {
            m_pageWords[p] = 0;
            continue;
        }

        const QString qText = QString::fromUtf8(UnicodeToUtf8(pageText).c_str());
        const QStringList tokens = qText.split(QRegularExpression(QStringLiteral("\\s+")),
                                               Qt::SkipEmptyParts);
        m_pageWords[p] = tokens.size();
        totalWords += tokens.size();

        const int ci = m_pageChapter[p];
        if (ci >= 0 && ci < m_chapters.size())
            m_chapters[ci].wordCount += tokens.size();
    }

    qDebug() << "ReadingTimeEstimator: computed word counts,"
             << m_chapters.size() << "chapters,"
             << totalWords << "total words,"
             << totalBookMinutes() << "minutes estimated";
}

int ReadingTimeEstimator::minutesRemainingInChapter(int currentPage0based) const
{
    if (m_chapters.isEmpty() || currentPage0based < 0 ||
            currentPage0based >= m_totalPages)
        return 0;

    const int ci = m_pageChapter.value(currentPage0based, -1);
    if (ci < 0 || ci >= m_chapters.size())
        return 0;

    const ChapterInfo &ch = m_chapters[ci];

    int wordsRemaining = 0;
    for (int p = currentPage0based; p <= ch.lastPage0 && p < m_totalPages; ++p) {
        wordsRemaining += m_pageWords.value(p, 0);
    }

    if (wordsRemaining <= 0)
        return 0;

    return static_cast<int>(
        std::ceil(static_cast<double>(wordsRemaining) / WORDS_PER_MINUTE));
}

int ReadingTimeEstimator::totalBookMinutes() const
{
    if (m_chapters.isEmpty())
        return 0;

    int totalWords = 0;
    for (const ChapterInfo &ch : m_chapters)
        totalWords += ch.wordCount;

    if (totalWords <= 0)
        return 0;

    return static_cast<int>(
        std::ceil(static_cast<double>(totalWords) / WORDS_PER_MINUTE));
}

void ReadingTimeEstimator::reset()
{
    m_chapters.clear();
    m_pageChapter.clear();
    m_pageWords.clear();
    m_totalPages = 0;
}
