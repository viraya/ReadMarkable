#include "AnnotationService.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QVariantMap>

AnnotationService::AnnotationService(QObject *parent)
    : QObject(parent)
{

    const QString dataDir = QStringLiteral("/home/root/.readmarkable/data");
    QDir dir;
    if (!dir.mkpath(dataDir)) {
        qWarning() << "AnnotationService: failed to create data directory" << dataDir;
    }

    const QString dbPath = dataDir + QStringLiteral("/annotations.db");
    m_repo = std::make_unique<AnnotationRepository>(dbPath);

    if (!m_repo->isOpen()) {
        qCritical() << "AnnotationService: database failed to open at" << dbPath;
    } else {
        qDebug() << "AnnotationService: ready, database at" << dbPath;
    }
}

int AnnotationService::annotationCount() const
{
    return m_currentAnnotations.size();
}

void AnnotationService::setCurrentBook(const QString &bookPath)
{
    if (bookPath == m_currentBookPath)
        return;

    m_currentBookPath = bookPath;
    refreshCache();

    qDebug() << "AnnotationService: current book set to"
             << bookPath << " - " << m_currentAnnotations.size() << "annotations";
}

int AnnotationService::saveHighlight(const QString &startXPtr,
                                     const QString &endXPtr,
                                     const QString &selectedText,
                                     int            style,
                                     const QString &chapterTitle)
{
    if (!m_repo || m_currentBookPath.isEmpty()) {
        qWarning() << "AnnotationService::saveHighlight: no current book";
        return -1;
    }
    if (startXPtr.isEmpty() || endXPtr.isEmpty()) {
        qWarning() << "AnnotationService::saveHighlight: empty XPointer";
        return -1;
    }

    if (m_repo->hasAnnotationAt(m_currentBookPath, startXPtr, endXPtr)) {
        qDebug() << "AnnotationService::saveHighlight: duplicate at" << startXPtr;
        return -1;
    }

    AnnotationEntry entry;

    {
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        hasher.addData(QFileInfo(m_currentBookPath).absoluteFilePath().toUtf8());
        entry.bookSha = QString::fromLatin1(hasher.result().toHex());
    }

    entry.startXPointer = startXPtr;
    entry.endXPointer   = endXPtr;
    entry.selectedText  = selectedText;
    entry.style         = static_cast<AnnotationStyle>(style);
    entry.note          = QString();
    entry.createdAt     = QDateTime::currentSecsSinceEpoch();
    entry.chapterTitle  = chapterTitle;

    const int64_t id = m_repo->insert(entry);
    if (id < 0) {
        qWarning() << "AnnotationService::saveHighlight: insert failed";
        return -1;
    }

    refreshCache();
    emit annotationsChanged();

    qDebug() << "AnnotationService: saved highlight id" << id
             << "text:" << selectedText.left(40);
    return static_cast<int>(id);
}

void AnnotationService::updateAnnotation(int annotationId,
                                         int style,
                                         const QString &note)
{
    if (!m_repo) return;

    const bool ok = m_repo->update(static_cast<int64_t>(annotationId),
                                   static_cast<AnnotationStyle>(style),
                                   note);
    if (!ok) {
        qWarning() << "AnnotationService::updateAnnotation: update failed for id"
                   << annotationId;
        return;
    }

    refreshCache();
    emit annotationsChanged();
}

void AnnotationService::deleteAnnotation(int annotationId)
{
    if (!m_repo) return;

    const bool ok = m_repo->remove(static_cast<int64_t>(annotationId));
    if (!ok) {
        qWarning() << "AnnotationService::deleteAnnotation: remove failed for id"
                   << annotationId;
        return;
    }

    refreshCache();
    emit annotationsChanged();
}

QVariantList AnnotationService::annotationsForCurrentBook() const
{
    QVariantList result;
    result.reserve(m_currentAnnotations.size());

    for (const AnnotationEntry &e : m_currentAnnotations) {
        QVariantMap map;
        map[QStringLiteral("id")]            = static_cast<int>(e.id);
        map[QStringLiteral("startXPointer")] = e.startXPointer;
        map[QStringLiteral("endXPointer")]   = e.endXPointer;
        map[QStringLiteral("selectedText")]  = e.selectedText;
        map[QStringLiteral("style")]         = static_cast<int>(e.style);
        map[QStringLiteral("note")]          = e.note;
        map[QStringLiteral("createdAt")]     = static_cast<int>(e.createdAt);
        map[QStringLiteral("chapterTitle")]  = e.chapterTitle;
        result.append(map);
    }

    return result;
}

bool AnnotationService::hasAnnotationAt(const QString &startXPtr,
                                        const QString &endXPtr) const
{
    if (!m_repo || m_currentBookPath.isEmpty())
        return false;

    return m_repo->hasAnnotationAt(m_currentBookPath, startXPtr, endXPtr);
}

bool AnnotationService::hasOverlappingText(const QString &text) const
{
    if (text.length() < 10)
        return false;

    for (const auto &ann : m_currentAnnotations) {
        if (ann.selectedText.length() < 10)
            continue;

        const bool aContainsB = ann.selectedText.contains(text);
        const bool bContainsA = text.contains(ann.selectedText);
        if (!aContainsB && !bContainsA)
            continue;

        const int shorter = qMin(ann.selectedText.length(), text.length());
        const int longer  = qMax(ann.selectedText.length(), text.length());
        if (shorter * 2 >= longer)
            return true;
    }
    return false;
}

void AnnotationService::refreshCache()
{
    if (!m_repo || m_currentBookPath.isEmpty()) {
        m_currentAnnotations.clear();
        return;
    }

    m_currentAnnotations = m_repo->loadForBook(m_currentBookPath);
}
