#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "payload/cache.h"

using rm::installer::payload::Cache;

class TestCache : public QObject {
    Q_OBJECT
 private slots:
    void lookupReturnsNullWhenMissing();
    void storeThenLookupRoundTrips();
    void storeOverwritesOldEntry();
    void evictKeepsThreeNewest();
};

namespace {

QString createFile(const QString& dir, const QString& name, const QByteArray& content) {
    QDir().mkpath(dir);
    const QString path = dir + "/" + name;
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(content);
    f.close();
    return path;
}

}  // namespace

void TestCache::lookupReturnsNullWhenMissing() {
    QTemporaryDir dir;
    Cache cache(dir.path());
    QVERIFY(!cache.lookup(QStringLiteral("v1.0.0")).has_value());
}

void TestCache::storeThenLookupRoundTrips() {
    QTemporaryDir dir;
    QTemporaryDir src;
    const QString srcFile = createFile(src.path(), "tmp.tar.gz",
                                       QByteArrayLiteral("tarballdata"));
    Cache cache(dir.path());
    QString err;
    QVERIFY2(cache.store(QStringLiteral("v1.2.0"), srcFile, &err), qPrintable(err));
    QVERIFY(!QFile::exists(srcFile));  // moved, not copied
    auto p = cache.lookup(QStringLiteral("v1.2.0"));
    QVERIFY(p.has_value());
    QVERIFY(QFile::exists(*p));
    QCOMPARE(QFileInfo(*p).fileName(),
             QStringLiteral("readmarkable-v1.2.0-chiappa.tar.gz"));
}

void TestCache::storeOverwritesOldEntry() {
    QTemporaryDir dir;
    Cache cache(dir.path());
    QTemporaryDir srcDir;
    const QString a = createFile(srcDir.path(), "a.tar.gz",
                                  QByteArrayLiteral("first"));
    QVERIFY(cache.store(QStringLiteral("v1.2.0"), a, nullptr));
    const QString b = createFile(srcDir.path(), "b.tar.gz",
                                  QByteArrayLiteral("secondversion"));
    QVERIFY(cache.store(QStringLiteral("v1.2.0"), b, nullptr));
    auto p = cache.lookup(QStringLiteral("v1.2.0"));
    QVERIFY(p.has_value());
    QFile f(*p);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArrayLiteral("secondversion"));
}

void TestCache::evictKeepsThreeNewest() {
    QTemporaryDir dir;
    Cache cache(dir.path());
    QTemporaryDir srcDir;
    // Populate 5 entries in oldest → newest order. Sleep a bit between to
    // guarantee distinct mtimes.
    for (int i = 0; i < 5; ++i) {
        const QString src = createFile(srcDir.path(),
                                       QString("s%1.tar.gz").arg(i),
                                       QByteArray(32, 'x'));
        QVERIFY(cache.store(QString("v%1.0.0").arg(i), src, nullptr));
        QTest::qWait(50);
    }
    cache.evict(3);
    const QDir d(dir.path());
    const auto entries = d.entryList(
        QStringList{"readmarkable-*-chiappa.tar.gz"},
        QDir::Files);
    QCOMPARE(entries.size(), 3);
    // The surviving three should be v2, v3, v4, the oldest two (v0, v1) gone.
    QVERIFY(!cache.lookup(QStringLiteral("v0.0.0")).has_value());
    QVERIFY(!cache.lookup(QStringLiteral("v1.0.0")).has_value());
    QVERIFY(cache.lookup(QStringLiteral("v2.0.0")).has_value());
    QVERIFY(cache.lookup(QStringLiteral("v3.0.0")).has_value());
    QVERIFY(cache.lookup(QStringLiteral("v4.0.0")).has_value());
}

QTEST_MAIN(TestCache)
#include "test_cache.moc"
