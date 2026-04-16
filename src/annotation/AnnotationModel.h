#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QString>
#include <QVariantList>
#include <QVector>

class AnnotationService;
class MarginNoteService;

class AnnotationModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum AnnotationRole {
        IdRole          = Qt::UserRole,
        StartXPtrRole,
        EndXPtrRole,
        SelectedTextRole,
        StyleRole,
        NoteRole,
        CreatedAtRole,
        ChapterTitleRole,
        IsMarginNoteRole,
        ThumbnailRectsRole,
        StrokeDataRole,
    };
    Q_ENUM(AnnotationRole)

    explicit AnnotationModel(QObject *parent = nullptr);

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refresh(AnnotationService *annotationService,
                 MarginNoteService *marginNoteService);

    void clear();

    int count() const;

    Q_INVOKABLE QString startXPointerAt(int index) const;

signals:
    void countChanged();

private:
    struct AnnotationDisplayEntry {
        int          id           = 0;
        QString      startXPointer;
        QString      endXPointer;
        QString      selectedText;
        int          style        = 0;
        QString      note;
        int          createdAt    = 0;
        QString      chapterTitle;
        bool         isMarginNote  = false;
        QVariantList thumbnailRects;
        QByteArray   strokeData;
        int          documentY    = 0;
    };

    QVector<AnnotationDisplayEntry> m_entries;
};
