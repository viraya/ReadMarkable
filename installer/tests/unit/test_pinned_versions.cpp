#include <QByteArray>
#include <QString>
#include <QtTest/QtTest>

#include "core/pinned_versions.h"

using rm::installer::core::PinnedConfig;

class TestPinnedVersions : public QObject {
    Q_OBJECT

 private slots:
    void loadsValidFixture();
    void rejectsMissingSection();
    void rejectsMissingKey();
    void rejectsMalformedToml();
    void defaultsSha256UrlTemplate();
    void resolvesReadMarkableUrl();
    void compareXoviTag();
    void loadsRealPinnedVersionsFromResource();
};

namespace {

constexpr const char* kValidToml = R"(
[xovi]
tag = "v18-23032026"
url = "https://example.test/xovi.tar.gz"
sha256 = "aaaa"
minimum_compatible_tag = "v17-01012026"

[appload]
tag = "v0.5.0"
url = "https://example.test/appload.zip"
sha256 = "bbbb"

[readmarkable]
url_template = "https://example.test/rm-{tag}.tar.gz"
min_version = "1.2.0"
)";

}  // namespace

void TestPinnedVersions::loadsValidFixture() {
    QString err;
    auto cfg = PinnedConfig::loadFromString(QByteArray(kValidToml), &err);
    QVERIFY2(cfg.has_value(), qPrintable(err));
    QCOMPARE(cfg->xovi.tag, QStringLiteral("v18-23032026"));
    QCOMPARE(cfg->xovi.sha256, QStringLiteral("aaaa"));
    QCOMPARE(cfg->xovi.minimumCompatibleTag, QStringLiteral("v17-01012026"));
    QCOMPARE(cfg->appload.tag, QStringLiteral("v0.5.0"));
    QCOMPARE(cfg->readmarkable.minVersion, QStringLiteral("1.2.0"));
    QVERIFY(cfg->readmarkable.urlTemplate.contains(QStringLiteral("{tag}")));
}

void TestPinnedVersions::rejectsMissingSection() {
    constexpr const char* missingRm = R"(
[xovi]
tag = "v18-23032026"
url = "u"
sha256 = "s"
minimum_compatible_tag = "v17-01012026"

[appload]
tag = "v0.5.0"
url = "u"
sha256 = "s"
)";
    QString err;
    auto cfg = PinnedConfig::loadFromString(QByteArray(missingRm), &err);
    QVERIFY(!cfg.has_value());
    QVERIFY(err.contains(QStringLiteral("readmarkable")));
}

void TestPinnedVersions::rejectsMissingKey() {
    constexpr const char* missingSha = R"(
[xovi]
tag = "v18-23032026"
url = "u"
minimum_compatible_tag = "v17-01012026"

[appload]
tag = "v0.5.0"
url = "u"
sha256 = "s"

[readmarkable]
url_template = "u"
min_version = "1.2.0"
)";
    QString err;
    auto cfg = PinnedConfig::loadFromString(QByteArray(missingSha), &err);
    QVERIFY(!cfg.has_value());
    QVERIFY(err.contains(QStringLiteral("sha256")));
}

void TestPinnedVersions::rejectsMalformedToml() {
    QByteArray broken("[xovi\ntag = \"v18\"\n");
    QString err;
    auto cfg = PinnedConfig::loadFromString(broken, &err);
    QVERIFY(!cfg.has_value());
    QVERIFY(!err.isEmpty());
}

void TestPinnedVersions::defaultsSha256UrlTemplate() {
    QString err;
    auto cfg = PinnedConfig::loadFromString(QByteArray(kValidToml), &err);
    QVERIFY(cfg.has_value());
    QCOMPARE(cfg->readmarkable.sha256UrlTemplate, QStringLiteral("{url}.sha256"));
}

void TestPinnedVersions::resolvesReadMarkableUrl() {
    QString err;
    auto cfg = PinnedConfig::loadFromString(QByteArray(kValidToml), &err);
    QVERIFY(cfg.has_value());
    QCOMPARE(cfg->resolveReadMarkableUrl(QStringLiteral("v1.2.0")),
             QStringLiteral("https://example.test/rm-v1.2.0.tar.gz"));
    QCOMPARE(cfg->resolveReadMarkableSha256Url(QStringLiteral("v1.2.0")),
             QStringLiteral("https://example.test/rm-v1.2.0.tar.gz.sha256"));
}

void TestPinnedVersions::compareXoviTag() {
    QVERIFY(PinnedConfig::compareXoviTag(QStringLiteral("v17-01012026"),
                                         QStringLiteral("v18-23032026")) < 0);
    QVERIFY(PinnedConfig::compareXoviTag(QStringLiteral("v18-23032026"),
                                         QStringLiteral("v18-23032026")) == 0);
    QVERIFY(PinnedConfig::compareXoviTag(QStringLiteral("v19-01012027"),
                                         QStringLiteral("v18-23032026")) > 0);
}

void TestPinnedVersions::loadsRealPinnedVersionsFromResource() {
    // Source-of-truth TOML lives at installer/config/pinned-versions.toml and
    // is also shipped as a Qt resource. Read from disk here (resources aren't
    // linked into the test binary, only the main executable).
    const QString path = QStringLiteral("%1/config/pinned-versions.toml")
                             .arg(QStringLiteral(RM_INSTALLER_SOURCE_DIR));
    QString err;
    auto cfg = PinnedConfig::loadFromFile(path, &err);
    QVERIFY2(cfg.has_value(), qPrintable(err));
    QVERIFY(!cfg->xovi.tag.isEmpty());
    QVERIFY(!cfg->xovi.sha256.isEmpty());
    QCOMPARE(cfg->xovi.sha256.size(), 64);  // hex-encoded SHA-256
    QCOMPARE(cfg->appload.sha256.size(), 64);
}

QTEST_MAIN(TestPinnedVersions)
#include "test_pinned_versions.moc"
