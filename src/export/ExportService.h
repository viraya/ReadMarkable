#pragma once

#include <QDate>
#include <QObject>
#include <QString>

class AnnotationService;
class MarginNoteService;
class BookmarkRepository;
class TocModel;

class ExportService : public QObject
{
    Q_OBJECT

public:
    explicit ExportService(AnnotationService  *annSvc,
                           MarginNoteService  *mnSvc,
                           BookmarkRepository *bmRepo,
                           TocModel           *tocModel,
                           QObject            *parent = nullptr);
    ~ExportService() override = default;

    Q_INVOKABLE void exportBook(const QString &bookPath,
                                const QString &bookTitle,
                                const QString &bookAuthor);

signals:

    void exportComplete(const QString &mdPath,
                        int highlightCount,
                        int noteCount,
                        int bookmarkCount);

    void exportFailed(const QString &reason);

private:

    static QString sanitizeFilename(const QString &s);

    AnnotationService  *m_annotationService;
    MarginNoteService  *m_marginNoteService;
    BookmarkRepository *m_bookmarkRepo;
    TocModel           *m_tocModel;

    static constexpr const char *kExportDir = "/home/root/.readmarkable/exports/";
};
