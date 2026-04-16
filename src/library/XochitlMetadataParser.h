#pragma once

#include "XochitlEntry.h"

#include <QList>
#include <QObject>
#include <QString>
#include <optional>

class QJsonObject;

class XochitlMetadataParser : public QObject {
    Q_OBJECT

public:

    explicit XochitlMetadataParser(QString xochitlDir, QObject *parent = nullptr);

    QList<XochitlEntry> scanAll();

    std::optional<XochitlEntry> parseEntry(const QString &uuid);

    const QString &xochitlDir() const { return m_xochitlDir; }

private:

    std::optional<QJsonObject> loadJsonWithRetry(const QString &path) const;

    void populateEpubDescendantCounts(QList<XochitlEntry> &entries) const;

    QString m_xochitlDir;
};
