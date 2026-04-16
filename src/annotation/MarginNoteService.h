#pragma once

#include "MarginNoteRepository.h"

#include <QByteArray>
#include <QObject>
#include <QPoint>
#include <QVariantList>
#include <QVector>
#include <memory>

class CrEngineRenderer;

class MarginNoteService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int          marginNoteCount  READ marginNoteCount  NOTIFY marginNotesChanged)
    Q_PROPERTY(QVariantList currentPageNotes READ currentPageNotes NOTIFY currentPageNotesChanged)

public:
    explicit MarginNoteService(QObject *parent = nullptr);
    ~MarginNoteService() override = default;

    void setRenderer(CrEngineRenderer *renderer);

    int          marginNoteCount()  const;
    QVariantList currentPageNotes() const;

    Q_INVOKABLE void setCurrentBook(const QString &bookPath);

    Q_INVOKABLE int saveMarginNote(const QString &paragraphXPtr,
                                   const QByteArray &strokeData);

    Q_INVOKABLE void updateMarginNoteStrokes(int noteId,
                                             const QByteArray &strokeData);

    Q_INVOKABLE void deleteMarginNote(int noteId);

    Q_INVOKABLE QVariantList marginNotesForCurrentBook() const;

    Q_INVOKABLE void loadNotesForPage();

    static QByteArray serializeStrokes(const QVector<QVector<QPoint>> &strokes);

    static QVector<QVector<QPoint>> deserializeStrokes(const QByteArray &data);

    static QVariantList generateThumbnailRects(const QByteArray &strokeData,
                                               int thumbW = 48, int thumbH = 48);

    Q_INVOKABLE QVariantList deserializeStrokesToRects(const QByteArray &strokeData) const;

    const QVector<MarginNoteEntry>& cachedNotes() const { return m_currentNotes; }

signals:

    void marginNotesChanged();

    void currentPageNotesChanged();

private:

    void refreshCache();

    int paragraphYPosition(const QString &xpointerStr) const;

    std::unique_ptr<MarginNoteRepository> m_repo;
    CrEngineRenderer *m_renderer = nullptr;

    QString                  m_currentBookPath;
    QVector<MarginNoteEntry> m_currentNotes;
    QVariantList             m_currentPageNotes;
};
