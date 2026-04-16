#pragma once

#include <QObject>
#include <QString>

class CrEngineRenderer;
class BookmarkRepository;
class TocModel;
class BookmarkModel;
class ReadingTimeEstimator;
class LVDocView;

class NavigationController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int     progressPercent            READ progressPercent            NOTIFY progressChanged)
    Q_PROPERTY(int     chapterProgressPercent     READ chapterProgressPercent     NOTIFY progressChanged)
    Q_PROPERTY(int     currentChapterIndex        READ currentChapterIndex        NOTIFY progressChanged)
    Q_PROPERTY(int     totalChapters              READ totalChapters              NOTIFY progressChanged)
    Q_PROPERTY(QString currentChapterTitle        READ currentChapterTitle        NOTIFY progressChanged)
    Q_PROPERTY(QString currentBookTitle           READ currentBookTitle           NOTIFY currentBookTitleChanged)
    Q_PROPERTY(int     minutesRemainingInChapter  READ minutesRemainingInChapter  NOTIFY progressChanged)

    Q_PROPERTY(bool    canGoBack           READ canGoBack           NOTIFY canGoBackChanged)

    Q_PROPERTY(bool    currentPageBookmarked READ currentPageBookmarked NOTIFY bookmarkStateChanged)

public:
    explicit NavigationController(CrEngineRenderer *renderer,
                                  BookmarkRepository *bookmarkRepo,
                                  QObject *parent = nullptr);
    ~NavigationController() override;

    int     progressPercent()            const;
    int     chapterProgressPercent()     const;
    int     currentChapterIndex()        const;
    int     totalChapters()              const;
    QString currentChapterTitle()        const;
    QString currentBookTitle()           const { return m_currentBookTitle; }
    bool    canGoBack()                  const;
    bool    currentPageBookmarked()      const;
    int     minutesRemainingInChapter()  const;

    TocModel      *tocModel()      const;
    BookmarkModel *bookmarkModel() const;

    Q_INVOKABLE void nextPage();

    Q_INVOKABLE void previousPage();

    Q_INVOKABLE void nextChapter();

    Q_INVOKABLE void previousChapter();

    Q_INVOKABLE void goToTocEntry(int index);

    Q_INVOKABLE void goToBookmarkXPointer(const QString &xpointer);

    Q_INVOKABLE void goBack();

    Q_INVOKABLE void toggleBookmark();

    Q_INVOKABLE QString checkLink(int contentX, int contentY);

    Q_INVOKABLE QString getLinkTargetText(const QString &href);

    Q_INVOKABLE void navigateLink(const QString &href);

    Q_INVOKABLE void refreshProgress();

    Q_INVOKABLE void refreshBookmarkState();

    void onDocumentLoaded(const QString &bookPath);

signals:

    void progressChanged();

    void canGoBackChanged();

    void bookmarkStateChanged();

    void currentBookTitleChanged();

    void internalLinkDetected(QString href, bool isFootnote);

    void linkNavigated();

private:
    CrEngineRenderer    *m_renderer;
    BookmarkRepository  *m_bookmarkRepo;
    TocModel            *m_tocModel;
    BookmarkModel       *m_bookmarkModel;
    ReadingTimeEstimator *m_readingTimeEstimator;

    int     m_progressPercent            = 0;
    int     m_currentChapterIndex        = -1;
    int     m_totalChapters              = 0;
    QString m_currentChapterTitle;
    bool    m_canGoBack                  = false;
    bool    m_currentPageBookmarked      = false;
    int     m_minutesRemainingInChapter  = 0;

    QString m_bookPath;

    QString m_currentBookTitle;

    LVDocView *docView() const;

    QString deriveBookTitleFromPath(const QString &path) const;
};
