#include <QByteArray>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "credentials/keypair.h"

using rm::installer::credentials::Ed25519Keypair;

class TestKeypair : public QObject {
    Q_OBJECT
 private slots:
    void generatesValidPair();
    void publicKeyParsesAsSshEd25519();
    void fingerprintIsStable();
    void saveAndLoadRoundTrip();
    void differentGenerationsDifferEverywhere();
};

void TestKeypair::generatesValidPair() {
    QString err;
    auto kp = Ed25519Keypair::generate(QStringLiteral("test@unit"), &err);
    QVERIFY2(kp.has_value(), qPrintable(err));
    // OpenSSH native format (what libssh2 actually loads); see keypair.cpp
    // `opensshPrivateKeyPem` for rationale.
    QVERIFY(kp->privateKeyPem.contains("BEGIN OPENSSH PRIVATE KEY"));
    QVERIFY(kp->privateKeyPem.contains("END OPENSSH PRIVATE KEY"));
    QVERIFY(kp->publicKeyOpenSsh.startsWith("ssh-ed25519 "));
    QVERIFY(kp->publicKeyOpenSsh.contains("test@unit"));
    QVERIFY(kp->fingerprintSha256.startsWith(QStringLiteral("SHA256:")));
}

void TestKeypair::publicKeyParsesAsSshEd25519() {
    auto kp = Ed25519Keypair::generate(QStringLiteral("x"));
    QVERIFY(kp.has_value());
    const QList<QByteArray> parts = kp->publicKeyOpenSsh.trimmed().split(' ');
    QCOMPARE(parts.size(), 3);  // "ssh-ed25519", base64, comment
    QCOMPARE(parts[0], QByteArrayLiteral("ssh-ed25519"));
    const QByteArray blob = QByteArray::fromBase64(parts[1]);
    // Wire format: [4-byte len=11]"ssh-ed25519"[4-byte len=32][32-byte pub]
    QCOMPARE(blob.size(), 4 + 11 + 4 + 32);
    QCOMPARE(blob.mid(4, 11), QByteArrayLiteral("ssh-ed25519"));
}

void TestKeypair::fingerprintIsStable() {
    auto kp = Ed25519Keypair::generate(QStringLiteral("a"));
    QVERIFY(kp.has_value());
    const QString fp1 = kp->fingerprintSha256;
    const QString fp2 = Ed25519Keypair::computeFingerprint(kp->publicKeyOpenSsh);
    QCOMPARE(fp1, fp2);
}

void TestKeypair::saveAndLoadRoundTrip() {
    QTemporaryDir dir;
    const QString priv = dir.path() + "/id_ed25519";
    const QString pub = dir.path() + "/id_ed25519.pub";
    auto kp = Ed25519Keypair::generate(QStringLiteral("comment"));
    QVERIFY(kp.has_value());
    QString err;
    QVERIFY2(kp->saveToFiles(priv, pub, &err), qPrintable(err));

    QVERIFY(QFile::exists(priv));
    QVERIFY(QFile::exists(pub));

    auto loaded = Ed25519Keypair::loadFromFiles(priv, pub, &err);
    QVERIFY2(loaded.has_value(), qPrintable(err));
    QCOMPARE(loaded->publicKeyOpenSsh, kp->publicKeyOpenSsh);
    QCOMPARE(loaded->privateKeyPem, kp->privateKeyPem);
    QCOMPARE(loaded->fingerprintSha256, kp->fingerprintSha256);
}

void TestKeypair::differentGenerationsDifferEverywhere() {
    auto a = Ed25519Keypair::generate(QStringLiteral("x"));
    auto b = Ed25519Keypair::generate(QStringLiteral("x"));
    QVERIFY(a.has_value() && b.has_value());
    QVERIFY(a->privateKeyPem != b->privateKeyPem);
    QVERIFY(a->publicKeyOpenSsh != b->publicKeyOpenSsh);
    QVERIFY(a->fingerprintSha256 != b->fingerprintSha256);
}

QTEST_MAIN(TestKeypair)
#include "test_keypair.moc"
