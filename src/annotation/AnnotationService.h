#pragma once

#include "AnnotationRepository.h"
#include "AnnotationTypes.h"

#include <QObject>
#include <QVariantList>
#include <QVector>
#include <memory>

class AnnotationService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int annotationCount
               READ annotationCount
               NOTIFY annotationsChanged)

public:
    explicit AnnotationService(QObject *parent = nullptr);
    ~AnnotationService() override = default;

    int annotationCount() const;

    Q_INVOKABLE void setCurrentBook(const QString &bookPath);

    Q_INVOKABLE int saveHighlight(const QString &startXPtr,
                                  const QString &endXPtr,
                                  const QString &selectedText,
                                  int            style,
                                  const QString &chapterTitle);

    Q_INVOKABLE void updateAnnotation(int annotationId,
                                      int style,
                                      const QString &note);

    Q_INVOKABLE void deleteAnnotation(int annotationId);

    Q_INVOKABLE QVariantList annotationsForCurrentBook() const;

    Q_INVOKABLE bool hasAnnotationAt(const QString &startXPtr,
                                     const QString &endXPtr) const;

    bool hasOverlappingText(const QString &text) const;

    const QVector<AnnotationEntry>& cachedAnnotations() const { return m_currentAnnotations; }

signals:

    void annotationsChanged();

private:

    void refreshCache();

    std::unique_ptr<AnnotationRepository> m_repo;

    QString                  m_currentBookPath;
    QVector<AnnotationEntry> m_currentAnnotations;
};
