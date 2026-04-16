#include <QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include "../src/library/XochitlEntry.h"
#include "../src/library/XochitlMetadataParser.h"

static void writeMetadata(const QString &dir, const QString &uuid,
                          const QString &type = QStringLiteral("DocumentType"),
                          const QString &visibleName = QStringLiteral("Test Entry"),
                          const QString &parent = QString(),
                          bool deleted = false)
{
    const QString path = dir + QLatin1Char('/') + uuid + QStringLiteral(".metadata");
    QFile f(path);
    QVERIFY2(f.open(QIODevice::WriteOnly | QIODevice::Text), qPrintable(f.errorString()));
    QTextStream ts(&f);
    ts << "{\n"
       << "  \"type\": \"" << type << "\",\n"
       << "  \"visibleName\": \"" << visibleName << "\",\n"
       << "  \"parent\": \"" << parent << "\",\n"
       << "  \"deleted\": " << (deleted ? "true" : "false") << ",\n"
       << "  \"lastOpened\": \"2024-01-15T09:23:47.000Z\",\n"
       << "  \"lastModified\": \"2024-01-10T08:00:00.000Z\"\n"
       << "}\n";
    f.close();
}

static void writeContent(const QString &dir, const QString &uuid,
                         const QString &fileType = QStringLiteral("epub"))
{
    const QString path = dir + QLatin1Char('/') + uuid + QStringLiteral(".content");
    QFile f(path);
    QVERIFY2(f.open(QIODevice::WriteOnly | QIODevice::Text), qPrintable(f.errorString()));
    QTextStream ts(&f);
    if (fileType.isNull()) {

        ts << "{\n"
           << "  \"version\": 1\n"
           << "}\n";
    } else {
        ts << "{\n"
           << "  \"fileType\": \"" << fileType << "\",\n"
           << "  \"version\": 1\n"
           << "}\n";
    }
    f.close();
}

static void touchFile(const QString &path)
{
    QFile f(path);
    QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
    f.close();
}

class XochitlMetadataParserTest : public QObject {
    Q_OBJECT

private slots:

    void test_validEpub()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("aaaaaaaa-0001-0001-0001-000000000001");

        writeMetadata(dir, uuid, QStringLiteral("DocumentType"),
                      QStringLiteral("My EPUB"), QString(), false);
        writeContent(dir, uuid, QStringLiteral("epub"));
        touchFile(dir + QLatin1Char('/') + uuid + QStringLiteral(".epub"));

        XochitlMetadataParser parser(dir);
        auto result = parser.parseEntry(uuid);

        QVERIFY2(result.has_value(), "parseEntry must return Some for a valid EPUB entry");
        QCOMPARE(result->type, QStringLiteral("DocumentType"));
        QCOMPARE(result->visibleName, QStringLiteral("My EPUB"));
        QVERIFY2(result->isEpub, "isEpub must be true when fileType==\"epub\"");
        QVERIFY2(!result->epubFilePath.isEmpty(), "epubFilePath must be set when isEpub");
        QVERIFY2(QFileInfo(result->epubFilePath).isAbsolute(),
                 "epubFilePath must be absolute");
    }

    void test_validFolder()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("bbbbbbbb-0002-0002-0002-000000000002");

        writeMetadata(dir, uuid, QStringLiteral("CollectionType"),
                      QStringLiteral("My Folder"), QString(), false);

        XochitlMetadataParser parser(dir);
        auto result = parser.parseEntry(uuid);

        QVERIFY2(result.has_value(), "parseEntry must return Some for a valid folder entry");
        QCOMPARE(result->type, QStringLiteral("CollectionType"));
        QVERIFY2(!result->isEpub, "isEpub must be false for CollectionType entries");
    }

    void test_deletedFlagSignal()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("cccccccc-0003-0003-0003-000000000003");

        writeMetadata(dir, uuid, QStringLiteral("DocumentType"),
                      QStringLiteral("Deleted Book"), QString(), true);
        writeContent(dir, uuid, QStringLiteral("epub"));

        XochitlMetadataParser parser(dir);
        auto result = parser.parseEntry(uuid);

        QVERIFY2(!result.has_value(),
                 "parseEntry must return nullopt when deleted:true in .metadata");
    }

    void test_parentTrashSignal()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("dddddddd-0004-0004-0004-000000000004");

        writeMetadata(dir, uuid, QStringLiteral("DocumentType"),
                      QStringLiteral("Trashed Book"), QStringLiteral("trash"), false);
        writeContent(dir, uuid, QStringLiteral("epub"));

        XochitlMetadataParser parser(dir);
        auto result = parser.parseEntry(uuid);

        QVERIFY2(!result.has_value(),
                 "parseEntry must return nullopt when parent==\"trash\"");
    }

    void test_tombstoneSignal()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("eeeeeeee-0005-0005-0005-000000000005");

        writeMetadata(dir, uuid, QStringLiteral("DocumentType"),
                      QStringLiteral("Tombstoned Book"), QString(), false);
        writeContent(dir, uuid, QStringLiteral("epub"));

        touchFile(dir + QLatin1Char('/') + uuid + QStringLiteral(".tombstone"));

        XochitlMetadataParser parser(dir);
        auto result = parser.parseEntry(uuid);

        QVERIFY2(!result.has_value(),
                 "parseEntry must return nullopt when .tombstone file exists");
    }

    void test_malformedJsonRetriesThenFails()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("ffffffff-0006-0006-0006-000000000006");

        const QString metaPath = dir + QLatin1Char('/') + uuid + QStringLiteral(".metadata");
        QFile f(metaPath);
        QVERIFY2(f.open(QIODevice::WriteOnly | QIODevice::Text), qPrintable(f.errorString()));
        f.write("{not json at all");
        f.close();

        XochitlMetadataParser parser(dir);

        QElapsedTimer timer;
        timer.start();

        auto result = parser.parseEntry(uuid);

        const qint64 elapsedMs = timer.elapsed();

        QVERIFY2(!result.has_value(),
                 "parseEntry must return nullopt for permanently malformed JSON");

        QVERIFY2(elapsedMs >= 10,
                 qPrintable(QStringLiteral("Expected retry delay >= 10 ms, got %1 ms")
                                .arg(elapsedMs)));
    }

    void test_epubFallbackDetection()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("11111111-0007-0007-0007-000000000007");

        writeMetadata(dir, uuid, QStringLiteral("DocumentType"),
                      QStringLiteral("Fallback EPUB"), QString(), false);

        writeContent(dir, uuid, QString());

        touchFile(dir + QLatin1Char('/') + uuid + QStringLiteral(".epub"));

        XochitlMetadataParser parser(dir);
        auto result = parser.parseEntry(uuid);

        QVERIFY2(result.has_value(), "parseEntry must return Some for fallback EPUB");
        QVERIFY2(result->isEpub,
                 "isEpub must be true when fileType is absent but .epub file exists");
    }

    void test_pdfEntryNotMarkedAsEpub()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("22222222-0008-0008-0008-000000000008");

        writeMetadata(dir, uuid, QStringLiteral("DocumentType"),
                      QStringLiteral("My PDF"), QString(), false);
        writeContent(dir, uuid, QStringLiteral("pdf"));

        XochitlMetadataParser parser(dir);
        auto result = parser.parseEntry(uuid);

        QVERIFY2(result.has_value(),
                 "parseEntry must return Some for a live PDF entry (it's a valid document)");
        QVERIFY2(!result->isEpub, "isEpub must be false for fileType==\"pdf\" entries");
    }

    void test_orphanEntry()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuid = QStringLiteral("33333333-0009-0009-0009-000000000009");
        const QString nonExistentParent = QStringLiteral("99999999-dead-dead-dead-deaddeaddead");

        writeMetadata(dir, uuid, QStringLiteral("DocumentType"),
                      QStringLiteral("Orphan EPUB"), nonExistentParent, false);
        writeContent(dir, uuid, QStringLiteral("epub"));
        touchFile(dir + QLatin1Char('/') + uuid + QStringLiteral(".epub"));

        XochitlMetadataParser parser(dir);

        auto result = parser.parseEntry(uuid);
        QVERIFY2(result.has_value(),
                 "parseEntry must return Some for an orphan entry (XOCH-04: treat as root)");
        QCOMPARE(result->parent, nonExistentParent);

        const QList<XochitlEntry> entries = parser.scanAll();
        bool found = false;
        for (const XochitlEntry &e : entries) {
            if (e.uuid == uuid) { found = true; break; }
        }
        QVERIFY2(found, "scanAll must include orphan entries (treat as root)");
    }

    void test_cycleDetection()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        const QString dir = tmpDir.path();
        const QString uuidA = QStringLiteral("44444444-000a-000a-000a-00000000000a");
        const QString uuidB = QStringLiteral("55555555-000b-000b-000b-00000000000b");

        writeMetadata(dir, uuidA, QStringLiteral("CollectionType"),
                      QStringLiteral("Folder A"), uuidB, false);
        writeMetadata(dir, uuidB, QStringLiteral("CollectionType"),
                      QStringLiteral("Folder B"), uuidA, false);

        XochitlMetadataParser parser(dir);

        QList<XochitlEntry> entries = parser.scanAll();

        QCOMPARE(entries.size(), 2);

        bool foundA = false, foundB = false;
        for (const XochitlEntry &e : entries) {
            if (e.uuid == uuidA) { foundA = true; }
            if (e.uuid == uuidB) { foundB = true; }

            QVERIFY2(e.epubDescendantCount >= 0,
                     "epubDescendantCount must be non-negative (cycle-safe DFS)");
        }
        QVERIFY2(foundA && foundB, "Both cyclic folder entries must appear in scanAll output");
    }
};

QTEST_MAIN(XochitlMetadataParserTest)
#include "XochitlMetadataParserTest.moc"
