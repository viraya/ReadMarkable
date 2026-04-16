#include <QByteArray>
#include <QtTest/QtTest>

#include "flow/device_state.h"

using rm::installer::flow::DeviceState;

class TestDeviceState : public QObject {
    Q_OBJECT
 private slots:
    void parsesStockChiappa();
    void parsesXoviOnlyNoRm();
    void parsesFullStack();
    void rejectsMalformed();
    void rejectsMissingModel();
    void isSupportedModelMatchesPaperProVariants();
};

namespace {

constexpr const char* kStock = R"({
  "device_model": "reMarkable Chiappa",
  "firmware": "20260310084634",
  "rootfs_mount": "ro",
  "xovi":  {"installed": false, "tag": "", "install_source": "", "path": "/home/root/xovi", "boot_persistence": false},
  "appload": {"installed": false, "tag": "", "install_source": ""},
  "readmarkable": {"installed": false, "version": ""}
})";

constexpr const char* kXoviOnly = R"({
  "device_model": "reMarkable Chiappa",
  "firmware": "20260310084634",
  "rootfs_mount": "ro",
  "xovi":  {"installed": true, "tag": "v18-23032026", "install_source": "readmarkable-installer", "path": "/home/root/xovi", "boot_persistence": true},
  "appload": {"installed": true, "tag": "v0.5.0", "install_source": "readmarkable-installer"},
  "readmarkable": {"installed": false, "version": ""}
})";

constexpr const char* kFullStack = R"({
  "device_model": "reMarkable Chiappa",
  "firmware": "20260310084634",
  "rootfs_mount": "ro",
  "xovi":  {"installed": true, "tag": "v18-23032026", "install_source": "readmarkable-installer", "path": "/home/root/xovi", "boot_persistence": true},
  "appload": {"installed": true, "tag": "v0.5.0", "install_source": "readmarkable-installer"},
  "readmarkable": {"installed": true, "version": "1.1.0"}
})";

}  // namespace

void TestDeviceState::parsesStockChiappa() {
    QString err;
    auto s = DeviceState::fromJson(QByteArray(kStock), &err);
    QVERIFY2(s.has_value(), qPrintable(err));
    QCOMPARE(s->deviceModel, QStringLiteral("reMarkable Chiappa"));
    QCOMPARE(s->rootfsMount, QStringLiteral("ro"));
    QVERIFY(!s->xovi.installed);
    QVERIFY(!s->appload.installed);
    QVERIFY(!s->readmarkable.installed);
}

void TestDeviceState::parsesXoviOnlyNoRm() {
    QString err;
    auto s = DeviceState::fromJson(QByteArray(kXoviOnly), &err);
    QVERIFY2(s.has_value(), qPrintable(err));
    QVERIFY(s->xovi.installed);
    QCOMPARE(s->xovi.tag, QStringLiteral("v18-23032026"));
    QCOMPARE(s->xovi.installSource, QStringLiteral("readmarkable-installer"));
    QVERIFY(s->xovi.bootPersistence);
    QVERIFY(s->appload.installed);
    QVERIFY(!s->readmarkable.installed);
}

void TestDeviceState::parsesFullStack() {
    QString err;
    auto s = DeviceState::fromJson(QByteArray(kFullStack), &err);
    QVERIFY2(s.has_value(), qPrintable(err));
    QVERIFY(s->xovi.installed);
    QVERIFY(s->readmarkable.installed);
    QCOMPARE(s->readmarkable.version, QStringLiteral("1.1.0"));
}

void TestDeviceState::rejectsMalformed() {
    QString err;
    auto s = DeviceState::fromJson(QByteArray("{not valid json"), &err);
    QVERIFY(!s.has_value());
    QVERIFY(!err.isEmpty());
}

void TestDeviceState::rejectsMissingModel() {
    constexpr const char* noModel = R"({"firmware":"x","rootfs_mount":"ro",
      "xovi":{"installed":false},"appload":{"installed":false},
      "readmarkable":{"installed":false}})";
    QString err;
    auto s = DeviceState::fromJson(QByteArray(noModel), &err);
    QVERIFY(!s.has_value());
    QVERIFY(err.contains(QStringLiteral("device_model")));
}

void TestDeviceState::isSupportedModelMatchesPaperProVariants() {
    DeviceState s;
    s.deviceModel = QStringLiteral("reMarkable Chiappa");
    QVERIFY(s.isSupportedModel());
    s.deviceModel = QStringLiteral("reMarkable Ferrari");
    QVERIFY(s.isSupportedModel());
    s.deviceModel = QStringLiteral("reMarkable 2");
    QVERIFY(!s.isSupportedModel());
    s.deviceModel = QStringLiteral("BOOX Note");
    QVERIFY(!s.isSupportedModel());
}

QTEST_MAIN(TestDeviceState)
#include "test_device_state.moc"
