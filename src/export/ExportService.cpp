#include "ExportService.h"

#include "StrokeRenderer.h"
#include "annotation/AnnotationService.h"
#include "annotation/AnnotationTypes.h"
#include "annotation/MarginNoteService.h"
#include "annotation/MarginNoteRepository.h"
#include "navigation/BookmarkRepository.h"
#include "navigation/TocModel.h"

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QTextStream>
#include <QVector>

ExportService::ExportService(AnnotationService  *annSvc,
                             MarginNoteService  *mnSvc,
                             BookmarkRepository *bmRepo,
                             TocModel           *tocModel,
                             QObject            *parent)
    : QObject(parent)
    , m_annotationService(annSvc)
    , m_marginNoteService(mnSvc)
    , m_bookmarkRepo(bmRepo)
    , m_tocModel(tocModel)
{
}

void ExportService::exportBook(const QString &bookPath,
                               const QString &bookTitle,
                               const QString &bookAuthor)
{

    const QString exportDir = QString::fromUtf8(kExportDir);
    if (!QDir().mkpath(exportDir)) {
        qCritical() << "ExportService: failed to create export directory:" << exportDir;
        emit exportFailed(QStringLiteral("Could not create export directory: ") + exportDir);
        return;
    }

    const QString dateStr        = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    const QString sanitizedTitle = sanitizeFilename(bookTitle);
    const QString filename       = sanitizedTitle + QStringLiteral(" - ") + dateStr + QStringLiteral(".md");
    const QString notesDirName   = sanitizedTitle + QStringLiteral(" - ") + dateStr + QStringLiteral("-notes");
    const QString notesDir       = exportDir + notesDirName + QStringLiteral("/");
    const QString mdPath         = exportDir + filename;

    m_annotationService->setCurrentBook(bookPath);
    const QVector<AnnotationEntry> &annotations = m_annotationService->cachedAnnotations();

    m_marginNoteService->setCurrentBook(bookPath);
    const QVector<MarginNoteEntry> &marginNotes = m_marginNoteService->cachedNotes();

    const QVector<BookmarkEntry> bookmarks = m_bookmarkRepo->load(bookPath);

    QVector<QString> chapterOrder;
    {
        const int rowCount = m_tocModel->rowCount();
        for (int i = 0; i < rowCount; ++i) {
            QModelIndex idx = m_tocModel->index(i, 0);
            QString title   = m_tocModel->data(idx, TocModel::TitleRole).toString();
            if (!title.isEmpty() && !chapterOrder.contains(title)) {
                chapterOrder.append(title);
            }
        }
    }

    QMap<QString, QVector<const AnnotationEntry *>> byChapter;
    QVector<const AnnotationEntry *> uncategorized;

    for (const AnnotationEntry &entry : annotations) {
        const QString &ch = entry.chapterTitle;
        if (ch.isEmpty()) {
            uncategorized.append(&entry);
        } else {
            byChapter[ch].append(&entry);
        }
    }

    bool noteDirCreated = false;
    QVector<QPair<int64_t, QString>> marginNotePaths;

    for (const MarginNoteEntry &mn : marginNotes) {
        if (mn.strokeData.isEmpty()) continue;

        if (!noteDirCreated) {
            if (!QDir().mkpath(notesDir)) {
                qWarning() << "ExportService: failed to create notes dir:" << notesDir;

                continue;
            }
            noteDirCreated = true;
        }

        QByteArray pngData = StrokeRenderer::renderToPng(mn.strokeData, 400, 400);
        if (pngData.isEmpty()) continue;

        const QString pngFilename = QStringLiteral("note-") + QString::number(mn.id) + QStringLiteral(".png");
        const QString pngPath     = notesDir + pngFilename;

        QFile pngFile(pngPath);
        if (pngFile.open(QIODevice::WriteOnly)) {
            pngFile.write(pngData);
            pngFile.close();

            marginNotePaths.append({mn.id, notesDirName + QStringLiteral("/") + pngFilename});
        } else {
            qWarning() << "ExportService: failed to write PNG:" << pngPath;
        }
    }

    QString mdContent;
    QTextStream ts(&mdContent);
    ts.setEncoding(QStringConverter::Utf8);

    const int annotationCount  = annotations.size();
    const int marginNoteCount  = marginNotes.size();
    const int bookmarkCount    = bookmarks.size();

    QString titleEsc  = bookTitle;  titleEsc.replace(QLatin1Char('"'), QLatin1String("\\\""));
    QString authorEsc = bookAuthor; authorEsc.replace(QLatin1Char('"'), QLatin1String("\\\""));

    ts << "---\n";
    ts << "title: \"" << titleEsc << "\"\n";
    ts << "author: \"" << authorEsc << "\"\n";
    ts << "exported: \"" << dateStr << "\"\n";
    ts << "highlights: " << annotationCount << "\n";
    ts << "margin_notes: " << marginNoteCount << "\n";
    ts << "bookmarks: " << bookmarkCount << "\n";
    ts << "---\n\n";

    ts << "# " << bookTitle << "\n\n";

    for (const QString &chapter : chapterOrder) {
        const QVector<const AnnotationEntry *> &entries = byChapter.value(chapter);
        if (entries.isEmpty()) continue;

        ts << "## " << chapter << "\n\n";

        for (const AnnotationEntry *entry : entries) {

            ts << "> " << entry->selectedText << "\n";

            static const char *kStyleNames[] = {
                "gray", "underline", "bracket", "dotted",
                "yellow", "green", "blue", "red", "orange"
            };
            const int styleIdx = static_cast<int>(entry->style);
            if (styleIdx >= 0 && styleIdx < 9) {
                ts << "> *[" << kStyleNames[styleIdx] << "]*\n";
            }

            if (!entry->note.isEmpty()) {
                ts << "\n**Note:** " << entry->note << "\n";
            }

            ts << "\n---\n\n";
        }
    }

    if (!uncategorized.isEmpty()) {
        ts << "## Other\n\n";
        for (const AnnotationEntry *entry : uncategorized) {
            ts << "> " << entry->selectedText << "\n";
            if (!entry->note.isEmpty()) {
                ts << "\n**Note:** " << entry->note << "\n";
            }
            ts << "\n---\n\n";
        }
    }

    for (auto it = byChapter.cbegin(); it != byChapter.cend(); ++it) {
        if (chapterOrder.contains(it.key())) continue;
        ts << "## " << it.key() << "\n\n";
        for (const AnnotationEntry *entry : it.value()) {
            ts << "> " << entry->selectedText << "\n";
            if (!entry->note.isEmpty()) {
                ts << "\n**Note:** " << entry->note << "\n";
            }
            ts << "\n---\n\n";
        }
    }

    if (!marginNotePaths.isEmpty()) {
        ts << "## Margin Notes\n\n";
        for (const auto &pair : marginNotePaths) {
            ts << "![margin note](" << pair.second << ")\n\n";
        }
    }

    if (!bookmarks.isEmpty()) {
        ts << "## Bookmarks\n\n";
        for (const BookmarkEntry &bm : bookmarks) {
            const int pct = (bm.percent + 50) / 100;
            ts << "- [Bookmark] at position " << pct << "%\n";
        }
        ts << "\n";
    }

    QFile mdFile(mdPath);
    if (!mdFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString reason = QStringLiteral("Could not open file for writing: ") + mdPath;
        qCritical() << "ExportService:" << reason;
        emit exportFailed(reason);
        return;
    }

    QTextStream fileTs(&mdFile);
    fileTs.setEncoding(QStringConverter::Utf8);
    fileTs << mdContent;
    mdFile.close();

    qInfo() << "ExportService: exported to" << mdPath
            << "highlights:" << annotationCount
            << "margin notes:" << marginNoteCount
            << "bookmarks:" << bookmarkCount;

    emit exportComplete(mdPath, annotationCount, marginNoteCount, bookmarkCount);
}

QString ExportService::sanitizeFilename(const QString &s)
{
    QString result;
    result.reserve(s.size());
    for (const QChar &ch : s) {

        if (ch == QLatin1Char('/') || ch == QLatin1Char('\\') ||
            ch == QLatin1Char(':') || ch == QLatin1Char('*')  ||
            ch == QLatin1Char('?') || ch == QLatin1Char('"')  ||
            ch == QLatin1Char('<') || ch == QLatin1Char('>')  ||
            ch == QLatin1Char('|'))
        {
            result += QLatin1Char('_');
        } else {
            result += ch;
        }
    }
    if (result.size() > 120) {
        result = result.left(120);
    }
    return result;
}
