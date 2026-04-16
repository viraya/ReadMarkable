#pragma once

#include "DictionaryTypes.h"

#include <QObject>
#include <QVariantList>

class QThread;
class DictWorker;

class DictionaryService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    explicit DictionaryService(QObject *parent = nullptr);
    ~DictionaryService() override;

    bool available() const { return m_available; }
    bool loading()   const { return m_loading;   }

    Q_INVOKABLE void lookup(const QString &word);

    Q_INVOKABLE void cancelLookup();

signals:

    void lookupComplete(const QString &word, const QVariantList &entries);

    void availableChanged();
    void loadingChanged();

    void doLookup(const QString &word);

private slots:

    void onResultReady(const QString &word, const DictResult &result);

    void onWorkerReady(bool dbAvailable);

private:
    void setAvailable(bool v);
    void setLoading(bool v);

    QThread    *m_workerThread = nullptr;
    DictWorker *m_worker       = nullptr;

    bool   m_available    = false;
    bool   m_loading      = false;
    QString m_pendingWord;
};
