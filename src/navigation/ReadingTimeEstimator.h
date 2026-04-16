#pragma once

#include <QObject>
#include <QVector>

class LVDocView;

class ReadingTimeEstimator : public QObject
{
    Q_OBJECT

public:
    static constexpr int WORDS_PER_MINUTE = 238;

    explicit ReadingTimeEstimator(QObject *parent = nullptr);

    void computeChapterWordCounts(LVDocView *docView);

    int minutesRemainingInChapter(int currentPage0based) const;

    int totalBookMinutes() const;

    void reset();

private:
    struct ChapterInfo {
        int firstPage0 = 0;
        int lastPage0  = 0;
        int wordCount  = 0;
    };

    QVector<ChapterInfo> m_chapters;

    QVector<int> m_pageChapter;

    QVector<int> m_pageWords;

    int m_totalPages = 0;
};
