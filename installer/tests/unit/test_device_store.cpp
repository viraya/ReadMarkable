#include <QDateTime>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "credentials/device_store.h"

using rm::installer::credentials::DeviceRecord;
using rm::installer::credentials::DeviceStore;

class TestDeviceStore : public QObject {
    Q_OBJECT
 private slots:
    void findReturnsNullWhenEmpty();
    void addThenFindRoundTrips();
    void removeDeletesDirectory();
    void allEnumeratesRecords();
    void pathsDerivedFromFingerprint();
    void sanitizedFingerprintIsFilesystemSafe();
};

namespace {

DeviceRecord makeRecord(const QString& fp) {
    DeviceRecord r;
    r.fingerprint = fp;
    r.lastIp = QStringLiteral("10.11.99.1");
    r.lastModel = QStringLiteral("reMarkable Chiappa");
    r.lastFirmware = QStringLiteral("20260310084634");
    r.lastConnected = QDateTime(QDate(2026, 4, 15), QTime(12, 0), Qt::UTC);
    r.keyComment = QStringLiteral("readmarkable-installer@10.11.99.1-test");
    return r;
}

}  // namespace

void TestDeviceStore::findReturnsNullWhenEmpty() {
    QTemporaryDir dir;
    DeviceStore store(dir.path());
    auto r = store.findByFingerprint(QStringLiteral("SHA256:nope"));
    QVERIFY(!r.has_value());
}

void TestDeviceStore::addThenFindRoundTrips() {
    QTemporaryDir dir;
    DeviceStore store(dir.path());
    const QString fp = QStringLiteral("SHA256:abc123=");
    const auto record = makeRecord(fp);
    QString err;
    QVERIFY2(store.add(record, &err), qPrintable(err));

    auto got = store.findByFingerprint(fp);
    QVERIFY(got.has_value());
    QCOMPARE(got->fingerprint, fp);
    QCOMPARE(got->lastIp, record.lastIp);
    QCOMPARE(got->lastModel, record.lastModel);
    QCOMPARE(got->keyComment, record.keyComment);
    QCOMPARE(got->lastConnected.toUTC(), record.lastConnected.toUTC());
    QCOMPARE(got->privateKeyPath, store.privateKeyPathFor(fp));
}

void TestDeviceStore::removeDeletesDirectory() {
    QTemporaryDir dir;
    DeviceStore store(dir.path());
    const QString fp = QStringLiteral("SHA256:xyz/abc=");
    QVERIFY(store.add(makeRecord(fp)));
    QVERIFY(store.findByFingerprint(fp).has_value());
    QVERIFY(store.remove(fp));
    QVERIFY(!store.findByFingerprint(fp).has_value());
    // Remove again is a no-op.
    QVERIFY(!store.remove(fp));
}

void TestDeviceStore::allEnumeratesRecords() {
    QTemporaryDir dir;
    DeviceStore store(dir.path());
    QVERIFY(store.add(makeRecord(QStringLiteral("SHA256:one=="))));
    QVERIFY(store.add(makeRecord(QStringLiteral("SHA256:two=="))));
    QVERIFY(store.add(makeRecord(QStringLiteral("SHA256:three=="))));
    const auto all = store.all();
    QCOMPARE(all.size(), 3);
}

void TestDeviceStore::pathsDerivedFromFingerprint() {
    QTemporaryDir dir;
    DeviceStore store(dir.path());
    const QString fp = QStringLiteral("SHA256:abcDef=");
    const QString d = store.deviceDirFor(fp);
    // No slashes / = / : in the sanitized dir name.
    const QString leaf = d.section(QLatin1Char('/'), -1);
    QVERIFY(!leaf.contains(QLatin1Char('/')));
    QVERIFY(!leaf.contains(QLatin1Char(':')));
    QVERIFY(!leaf.contains(QLatin1Char('=')));
}

void TestDeviceStore::sanitizedFingerprintIsFilesystemSafe() {
    QTemporaryDir dir;
    DeviceStore store(dir.path());
    const QString fp = QStringLiteral("SHA256:tricky/chars=with=equals");
    QString err;
    QVERIFY2(store.add(makeRecord(fp), &err), qPrintable(err));
    QVERIFY(store.findByFingerprint(fp).has_value());
}

QTEST_MAIN(TestDeviceStore)
#include "test_device_store.moc"
