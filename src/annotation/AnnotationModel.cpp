#include "AnnotationModel.h"
#include "AnnotationService.h"
#include "MarginNoteService.h"

#include <QDebug>
#include <algorithm>

AnnotationModel::AnnotationModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AnnotationModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant AnnotationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const AnnotationDisplayEntry &entry = m_entries[index.row()];
    switch (role) {
    case IdRole:
        return entry.id;
    case StartXPtrRole:
        return entry.startXPointer;
    case EndXPtrRole:
        return entry.endXPointer;
    case SelectedTextRole:
        return entry.selectedText;
    case StyleRole:
        return entry.style;
    case NoteRole:
        return entry.note;
    case CreatedAtRole:
        return entry.createdAt;
    case ChapterTitleRole:
        return entry.chapterTitle;
    case IsMarginNoteRole:
        return entry.isMarginNote;
    case ThumbnailRectsRole:
        return entry.thumbnailRects;
    case StrokeDataRole:
        return entry.strokeData;
    default:
        return {};
    }
}

QHash<int, QByteArray> AnnotationModel::roleNames() const
{
    return {
        { IdRole,           "annotationId"   },
        { StartXPtrRole,    "startXPointer"  },
        { EndXPtrRole,      "endXPointer"    },
        { SelectedTextRole, "selectedText"   },
        { StyleRole,        "style"          },
        { NoteRole,         "note"           },
        { CreatedAtRole,    "createdAt"      },
        { ChapterTitleRole, "chapterTitle"   },
        { IsMarginNoteRole,   "isMarginNote"   },
        { ThumbnailRectsRole, "thumbnailRects" },
        { StrokeDataRole,     "strokeData"     },
    };
}

void AnnotationModel::refresh(AnnotationService *annotationService,
                               MarginNoteService *marginNoteService)
{
    beginResetModel();
    m_entries.clear();

    if (annotationService) {
        const QVariantList anns = annotationService->annotationsForCurrentBook();
        m_entries.reserve(m_entries.size() + anns.size());

        for (const QVariant &var : anns) {
            const QVariantMap m = var.toMap();

            AnnotationDisplayEntry e;
            e.id            = m.value(QStringLiteral("id"),           0).toInt();
            e.startXPointer = m.value(QStringLiteral("startXPointer")).toString();
            e.endXPointer   = m.value(QStringLiteral("endXPointer")).toString();
            e.selectedText  = m.value(QStringLiteral("selectedText")).toString();
            e.style         = m.value(QStringLiteral("style"),        0).toInt();
            e.note          = m.value(QStringLiteral("note")).toString();
            e.createdAt     = m.value(QStringLiteral("createdAt"),    0).toInt();
            e.chapterTitle  = m.value(QStringLiteral("chapterTitle")).toString();
            e.isMarginNote  = false;

            e.documentY     = m.contains(QStringLiteral("documentY"))
                              ? m.value(QStringLiteral("documentY")).toInt()
                              : e.createdAt;

            m_entries.append(e);
        }
    }

    if (marginNoteService) {
        const QVariantList notes = marginNoteService->marginNotesForCurrentBook();

        for (const QVariant &var : notes) {
            const QVariantMap m = var.toMap();

            AnnotationDisplayEntry e;
            e.id             = m.value(QStringLiteral("id"),        0).toInt();
            e.startXPointer  = m.value(QStringLiteral("paragraphXPointer")).toString();
            e.endXPointer    = QStringLiteral("");
            e.selectedText   = QStringLiteral("[Handwritten note]");
            e.style          = -1;
            e.note           = QStringLiteral("");
            e.createdAt      = m.value(QStringLiteral("createdAt"), 0).toInt();
            e.chapterTitle   = QStringLiteral("");
            e.isMarginNote   = true;
            e.strokeData     = m.value(QStringLiteral("strokeData")).toByteArray();
            e.thumbnailRects = MarginNoteService::generateThumbnailRects(e.strokeData);
            e.documentY      = m.value(QStringLiteral("paragraphY"), 0).toInt();

            m_entries.append(e);
        }
    }

    std::sort(m_entries.begin(), m_entries.end(),
              [](const AnnotationDisplayEntry &a, const AnnotationDisplayEntry &b) {
                  return a.documentY < b.documentY;
              });

    endResetModel();
    emit countChanged();

    qDebug() << "AnnotationModel::refresh: loaded" << m_entries.size()
             << "entries (annotations + margin notes)";
}

void AnnotationModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

int AnnotationModel::count() const
{
    return m_entries.size();
}

QString AnnotationModel::startXPointerAt(int index) const
{
    if (index < 0 || index >= m_entries.size())
        return {};
    return m_entries[index].startXPointer;
}
