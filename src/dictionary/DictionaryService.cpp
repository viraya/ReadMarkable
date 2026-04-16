#include "DictionaryService.h"
#include "DictionaryDatabase.h"

#include <QDebug>
#include <QDir>
#include <QThread>
#include <QVariantMap>

class DictWorker : public QObject
{
    Q_OBJECT

public:
    explicit DictWorker(const QString &dbPath, QObject *parent = nullptr)
        : QObject(parent)
        , m_dbPath(dbPath)
    {}

public slots:

    void init()
    {
        m_db = std::make_unique<DictionaryDatabase>(m_dbPath);
        emit ready(m_db->isAvailable());
    }

    void lookup(const QString &word)
    {
        DictResult result;

        if (m_db) {
            result = m_db->lookup(word);
        } else {
            result.word  = word;
            result.found = false;
        }

        emit resultReady(word, result);
    }

signals:

    void ready(bool dbAvailable);

    void resultReady(const QString &word, const DictResult &result);

private:
    QString                          m_dbPath;
    std::unique_ptr<DictionaryDatabase> m_db;
};

#include "DictionaryService.moc"

static const QString kDictDbPath =
    QStringLiteral("/home/root/.readmarkable/data/dict-en.db");

DictionaryService::DictionaryService(QObject *parent)
    : QObject(parent)
{

    QDir dir;
    dir.mkpath(QStringLiteral("/home/root/.readmarkable/data"));

    m_worker = new DictWorker(kDictDbPath);
    m_workerThread = new QThread(this);

    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started,
            m_worker,       &DictWorker::init);

    connect(m_worker, &DictWorker::ready,
            this,     &DictionaryService::onWorkerReady,
            Qt::QueuedConnection);

    connect(this,     &DictionaryService::doLookup,
            m_worker, &DictWorker::lookup,
            Qt::QueuedConnection);

    connect(m_worker, &DictWorker::resultReady,
            this,     &DictionaryService::onResultReady,
            Qt::QueuedConnection);

    connect(m_workerThread, &QThread::finished,
            m_worker,       &QObject::deleteLater);

    m_workerThread->start();

    qDebug() << "DictionaryService: worker thread started, awaiting db init";
}

DictionaryService::~DictionaryService()
{
    m_workerThread->quit();
    m_workerThread->wait(3000);

}

void DictionaryService::lookup(const QString &word)
{
    const QString trimmed = word.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    m_pendingWord = trimmed;
    setLoading(true);

    emit doLookup(trimmed);
}

void DictionaryService::cancelLookup()
{

    m_pendingWord.clear();
    setLoading(false);
}

void DictionaryService::onResultReady(const QString &word, const DictResult &result)
{

    if (word != m_pendingWord) {
        qDebug() << "DictionaryService: discarding stale result for" << word
                 << "(pending:" << m_pendingWord << ")";
        return;
    }

    m_pendingWord.clear();
    setLoading(false);

    if (!result.found) {
        qDebug() << "DictionaryService: no entry found for" << word;

        emit lookupComplete(word, QVariantList{});
        return;
    }

    QVariantList entries;
    entries.reserve(result.entries.size());

    for (const DictEntry &e : result.entries) {
        QVariantMap map;
        map[QStringLiteral("word")]          = e.word;
        map[QStringLiteral("partOfSpeech")]  = e.partOfSpeech;
        map[QStringLiteral("definition")]    = e.definition;
        map[QStringLiteral("ipa")]           = e.ipa;
        map[QStringLiteral("example")]       = e.example;
        entries.append(std::move(map));
    }

    qDebug() << "DictionaryService: lookup complete for" << word
             << " - " << entries.size() << "entries";

    emit lookupComplete(word, entries);
}

void DictionaryService::onWorkerReady(bool dbAvailable)
{
    setAvailable(dbAvailable);

    if (dbAvailable) {
        qDebug() << "DictionaryService: dictionary database ready";
    } else {
        qInfo() << "DictionaryService: dictionary database not available"
                << " -  run tools/build-dictionary-db.py to enable dictionary lookups";
    }
}

void DictionaryService::setAvailable(bool v)
{
    if (m_available == v) return;
    m_available = v;
    emit availableChanged();
}

void DictionaryService::setLoading(bool v)
{
    if (m_loading == v) return;
    m_loading = v;
    emit loadingChanged();
}
