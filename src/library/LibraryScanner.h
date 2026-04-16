#pragma once

#include "BookRecord.h"

#include <QObject>
#include <QString>

class PositionRepository;

class LibraryScanner : public QObject
{
    Q_OBJECT

public:

    explicit LibraryScanner(const QString       &libraryPath,
                            PositionRepository  *positionRepo,
                            QObject             *parent = nullptr);

    Q_INVOKABLE void scan();

    Q_INVOKABLE void scanDirectory(const QString &dirPath);

signals:

    void bookFound(const BookRecord &record);

    void scanComplete();

private:

    BookRecord buildBookRecord(const QString &epubPath) const;

    static int countEpubsInDirectory(const QString &dirPath);

    QString            m_libraryPath;
    PositionRepository *m_positionRepo;
};
