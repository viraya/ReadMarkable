#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ssh/known_hosts.h"

using rm::installer::ssh::KnownHosts;
using rm::installer::ssh::KnownHostsStatus;

class TestKnownHosts : public QObject {
    Q_OBJECT
 private slots:
    void newHostOnEmptyStore();
    void addThenMatch();
    void mismatchAfterRotation();
    void removeClearsEntry();
    void persistsAcrossInstances();
    void caseInsensitiveHost();
    void ignoresCommentsAndBlankLines();
};

void TestKnownHosts::newHostOnEmptyStore() {
    QTemporaryDir dir;
    KnownHosts kh(dir.path() + "/known_hosts");
    QCOMPARE(kh.verify("10.11.99.1", "SHA256:abc"), KnownHostsStatus::NewHost);
    QCOMPARE(kh.size(), 0);
}

void TestKnownHosts::addThenMatch() {
    QTemporaryDir dir;
    KnownHosts kh(dir.path() + "/known_hosts");
    QVERIFY(kh.add("10.11.99.1", "SHA256:abc"));
    QCOMPARE(kh.verify("10.11.99.1", "SHA256:abc"), KnownHostsStatus::Match);
    QCOMPARE(kh.size(), 1);
}

void TestKnownHosts::mismatchAfterRotation() {
    QTemporaryDir dir;
    KnownHosts kh(dir.path() + "/known_hosts");
    QVERIFY(kh.add("10.11.99.1", "SHA256:abc"));
    QCOMPARE(kh.verify("10.11.99.1", "SHA256:xyz"), KnownHostsStatus::Mismatch);
}

void TestKnownHosts::removeClearsEntry() {
    QTemporaryDir dir;
    KnownHosts kh(dir.path() + "/known_hosts");
    QVERIFY(kh.add("10.11.99.1", "SHA256:abc"));
    QVERIFY(kh.remove("10.11.99.1"));
    QCOMPARE(kh.verify("10.11.99.1", "SHA256:abc"), KnownHostsStatus::NewHost);
    QVERIFY(!kh.remove("10.11.99.1"));  // already gone → false
}

void TestKnownHosts::persistsAcrossInstances() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/known_hosts";
    {
        KnownHosts kh(path);
        QVERIFY(kh.add("remarkable.local", "SHA256:persistent"));
    }
    KnownHosts kh2(path);
    QCOMPARE(kh2.verify("remarkable.local", "SHA256:persistent"),
             KnownHostsStatus::Match);
}

void TestKnownHosts::caseInsensitiveHost() {
    QTemporaryDir dir;
    KnownHosts kh(dir.path() + "/known_hosts");
    QVERIFY(kh.add("REMARKABLE.local", "SHA256:abc"));
    QCOMPARE(kh.verify("remarkable.LOCAL", "SHA256:abc"), KnownHostsStatus::Match);
}

void TestKnownHosts::ignoresCommentsAndBlankLines() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/known_hosts";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("# comment\n\n10.11.99.1\tSHA256:abc\n# trailing\n");
    f.close();
    KnownHosts kh(path);
    QCOMPARE(kh.verify("10.11.99.1", "SHA256:abc"), KnownHostsStatus::Match);
    QCOMPARE(kh.size(), 1);
}

QTEST_MAIN(TestKnownHosts)
#include "test_known_hosts.moc"
