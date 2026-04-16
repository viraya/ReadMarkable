#pragma once

#include "SearchTypes.h"

#include <QObject>
#include <QVariantList>
#include <QVector>
#include <memory>

class SearchDatabase;
class CrEngineRenderer;
class TocModel;

class SearchService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList results      READ results      NOTIFY resultsChanged)
    Q_PROPERTY(int          resultCount  READ resultCount  NOTIFY resultsChanged)
    Q_PROPERTY(bool         indexReady   READ indexReady   NOTIFY indexReadyChanged)

public:
    explicit SearchService(QObject *parent = nullptr);
    ~SearchService() override;

    QVariantList results() const;

    int resultCount() const { return m_results.size(); }

    bool indexReady() const { return m_indexReady; }

    Q_INVOKABLE void setCurrentBook(const QString &bookPath);

    Q_INVOKABLE void buildIndex(CrEngineRenderer *renderer, TocModel *tocModel);

    Q_INVOKABLE void search(const QString &query);

    Q_INVOKABLE void clearResults();

    Q_INVOKABLE int pageForResultAt(int index) const;

    Q_INVOKABLE QVariantList findTextRects(const QString &query,
                                           CrEngineRenderer *renderer) const;

signals:
    void resultsChanged();
    void indexReadyChanged();

private:
    std::unique_ptr<SearchDatabase> m_db;

    QVector<SearchResult> m_results;
    QString               m_currentBookPath;
    QString               m_currentBookSha;
    bool                  m_indexReady = false;
};
