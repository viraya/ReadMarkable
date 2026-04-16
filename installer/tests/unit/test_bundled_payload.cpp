#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QtTest/QtTest>

#include "core/pinned_versions.h"
#include "payload/bundled_payload.h"

using rm::installer::core::PinnedConfig;
using rm::installer::payload::BundledPayload;

class TestBundledPayload : public QObject {
    Q_OBJECT

 private slots:
    void sha256KnownVectors();
    void extractAllSucceedsWithRealPins();
    void extractAllFailsOnTamperedPinnedHash();
    void onDeviceScriptsAreAllExtracted();
    void homeTileFilesAreExtracted();
};

namespace {

PinnedConfig loadRealPins() {
    const QString path = QStringLiteral("%1/config/pinned-versions.toml")
                             .arg(QStringLiteral(RM_INSTALLER_SOURCE_DIR));
    QString err;
    auto cfg = PinnedConfig::loadFromFile(path, &err);
    if (!cfg) {
        qFatal("cannot load real pinned-versions.toml: %s", qPrintable(err));
    }
    return *cfg;
}

}  // namespace

void TestBundledPayload::sha256KnownVectors() {
    // NIST sha256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    QCOMPARE(BundledPayload::sha256OfBytes(QByteArray()),
             QByteArray("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991"
                        "b7852b855"));
    QCOMPARE(BundledPayload::sha256OfBytes(QByteArray("abc")),
             QByteArray("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61"
                        "f20015ad"));
}

void TestBundledPayload::extractAllSucceedsWithRealPins() {
    PinnedConfig pinned = loadRealPins();
    BundledPayload payload(pinned);
    QString err;
    const bool ok = payload.extractAll(&err);
    QVERIFY2(ok, qPrintable(err));

    const auto& paths = payload.paths();
    QVERIFY(QFile::exists(paths.xoviTarball));
    QVERIFY(QFile::exists(paths.apploadZip));
    QVERIFY(QFileInfo(paths.xoviTarball).size() > 1024);  // real payload > 1 KB
    QVERIFY(QFileInfo(paths.apploadZip).size() > 1024);

    // Re-verify sha256 independently.
    QCOMPARE(QString::fromUtf8(BundledPayload::sha256OfFile(paths.xoviTarball)),
             pinned.xovi.sha256.toLower());
    QCOMPARE(QString::fromUtf8(BundledPayload::sha256OfFile(paths.apploadZip)),
             pinned.appload.sha256.toLower());
}

void TestBundledPayload::extractAllFailsOnTamperedPinnedHash() {
    PinnedConfig pinned = loadRealPins();
    // Flip the expected sha256 so extraction must reject the payload.
    pinned.xovi.sha256 = QStringLiteral(
        "0000000000000000000000000000000000000000000000000000000000000000");
    BundledPayload payload(pinned);
    QString err;
    const bool ok = payload.extractAll(&err);
    QVERIFY(!ok);
    QVERIFY2(err.contains(QStringLiteral("sha256")), qPrintable(err));
}

void TestBundledPayload::onDeviceScriptsAreAllExtracted() {
    PinnedConfig pinned = loadRealPins();
    BundledPayload payload(pinned);
    QString err;
    QVERIFY2(payload.extractAll(&err), qPrintable(err));

    const QStringList& files = payload.paths().onDeviceFiles;
    const QStringList expectedBasenames = {
        QStringLiteral("detect-state.sh"),
        QStringLiteral("install-xovi.sh"),
        QStringLiteral("install-appload.sh"),
        QStringLiteral("install-trigger.sh"),
        QStringLiteral("install-readmarkable.sh"),
        QStringLiteral("install-home-tile.sh"),
        QStringLiteral("rebuild-hashtab.sh"),
        QStringLiteral("uninstall.sh"),
        QStringLiteral("xovi-trigger.service"),
        QStringLiteral("xovi-trigger"),
    };
    QCOMPARE(files.size(), expectedBasenames.size());
    for (const QString& expected : expectedBasenames) {
        bool found = false;
        for (const QString& f : files) {
            if (QFileInfo(f).fileName() == expected && QFile::exists(f)) {
                found = true;
                break;
            }
        }
        QVERIFY2(found, qPrintable(QStringLiteral("missing: %1").arg(expected)));
    }
}

void TestBundledPayload::homeTileFilesAreExtracted() {
    PinnedConfig pinned = loadRealPins();
    BundledPayload payload(pinned);
    QString err;
    QVERIFY2(payload.extractAll(&err), qPrintable(err));

    const auto& paths = payload.paths();
    QVERIFY2(!paths.homeTileDir.isEmpty(), "homeTileDir should be set");
    QVERIFY2(QDir(paths.homeTileDir).exists(),
             qPrintable(QStringLiteral("home-tile dir missing: ") +
                        paths.homeTileDir));

    const QStringList& files = paths.homeTileFiles;
    const QStringList expected = {
        QStringLiteral("manifest.toml"),
        QStringLiteral("readmarkable-icon.png"),
        QStringLiteral("readmarkable-3.26.0.68.qmd"),
    };
    QCOMPARE(files.size(), expected.size());
    for (const QString& e : expected) {
        bool found = false;
        for (const QString& f : files) {
            if (QFileInfo(f).fileName() == e && QFile::exists(f) &&
                QFileInfo(f).size() > 0) {
                found = true;
                break;
            }
        }
        QVERIFY2(found, qPrintable(QStringLiteral("home-tile missing: ") + e));
    }
}

QTEST_MAIN(TestBundledPayload)
#include "test_bundled_payload.moc"
