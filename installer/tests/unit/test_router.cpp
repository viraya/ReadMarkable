#include <QtTest/QtTest>

#include "core/pinned_versions.h"
#include "flow/device_state.h"
#include "flow/router.h"

using rm::installer::core::PinnedConfig;
using rm::installer::flow::BlockingReason;
using rm::installer::flow::DeviceState;
using rm::installer::flow::FlowVariant;
using rm::installer::flow::decideInstallOrUpdate;
using rm::installer::flow::decideUninstall;

class TestRouter : public QObject {
    Q_OBJECT
 private slots:
    void freshDeviceRoutesToInstallFull();
    void xoviOnlyCompatibleRoutesToInstallSkipXovi();
    void xoviOnlyUnknownVersionRoutesToInstallSkipAll();
    void xoviOnlyTooOldRoutesToBlockingError();
    void rmAndXoviCompatibleRoutesToUpdate();
    void rmWithoutXoviIsBlockingError();
    void unsupportedModelIsBlockingError();
    void offersBootPersistenceWhenMissing();
    void uninstallShowsRemoveXoviCheckboxOnlyForOurInstall();
    void updateSummaryStripsLeadingVPrefix();
    void uninstallSummaryStripsLeadingVPrefix();
};

namespace {

PinnedConfig stubPinned() {
    PinnedConfig p;
    p.xovi.tag = QStringLiteral("v18-23032026");
    p.xovi.minimumCompatibleTag = QStringLiteral("v17-01012026");
    p.appload.tag = QStringLiteral("v0.5.0");
    p.readmarkable.minVersion = QStringLiteral("1.2.0");
    return p;
}

DeviceState freshChiappa() {
    DeviceState s;
    s.deviceModel = QStringLiteral("reMarkable Chiappa");
    s.firmware = QStringLiteral("20260310084634");
    s.rootfsMount = QStringLiteral("ro");
    return s;
}

}  // namespace

void TestRouter::freshDeviceRoutesToInstallFull() {
    auto d = decideInstallOrUpdate(freshChiappa(), stubPinned());
    QCOMPARE(d.variant, FlowVariant::Install);
    QCOMPARE(d.blockingReason, BlockingReason::None);
    QVERIFY(!d.skipXoviInstall);
    QVERIFY(!d.skipAppLoadInstall);
    QVERIFY(!d.needsAppLoadInstall);
}

void TestRouter::xoviOnlyCompatibleRoutesToInstallSkipXovi() {
    DeviceState s = freshChiappa();
    s.xovi.installed = true;
    s.xovi.tag = QStringLiteral("v18-23032026");
    s.xovi.bootPersistence = true;
    // AppLoad NOT installed  -  we should offer to install it.
    auto d = decideInstallOrUpdate(s, stubPinned());
    QCOMPARE(d.variant, FlowVariant::Install);
    QVERIFY(d.skipXoviInstall);
    QVERIFY(!d.skipAppLoadInstall);
    QVERIFY(d.needsAppLoadInstall);
}

void TestRouter::xoviOnlyUnknownVersionRoutesToInstallSkipAll() {
    DeviceState s = freshChiappa();
    s.xovi.installed = true;
    s.xovi.tag = QStringLiteral("unknown");
    s.xovi.installSource = QStringLiteral("user");
    s.xovi.bootPersistence = true;
    auto d = decideInstallOrUpdate(s, stubPinned());
    QCOMPARE(d.variant, FlowVariant::Install);
    QVERIFY(d.skipXoviInstall);
    QVERIFY(d.skipAppLoadInstall);  // unknown XOVI → don't touch AppLoad either
}

void TestRouter::xoviOnlyTooOldRoutesToBlockingError() {
    DeviceState s = freshChiappa();
    s.xovi.installed = true;
    s.xovi.tag = QStringLiteral("v16-01012025");  // older than v17-01012026 pin
    auto d = decideInstallOrUpdate(s, stubPinned());
    QCOMPARE(d.variant, FlowVariant::BlockingError);
    QCOMPARE(d.blockingReason, BlockingReason::XoviTooOld);
    QVERIFY(!d.errorDetails.isEmpty());
}

void TestRouter::rmAndXoviCompatibleRoutesToUpdate() {
    DeviceState s = freshChiappa();
    s.xovi.installed = true;
    s.xovi.tag = QStringLiteral("v18-23032026");
    s.xovi.bootPersistence = true;
    s.appload.installed = true;
    s.appload.tag = QStringLiteral("v0.5.0");
    s.readmarkable.installed = true;
    s.readmarkable.version = QStringLiteral("1.1.0");
    auto d = decideInstallOrUpdate(s, stubPinned());
    QCOMPARE(d.variant, FlowVariant::Update);
    QVERIFY(d.skipXoviInstall);
    QVERIFY(d.skipAppLoadInstall);
}

void TestRouter::rmWithoutXoviIsBlockingError() {
    DeviceState s = freshChiappa();
    s.xovi.installed = false;
    s.readmarkable.installed = true;
    s.readmarkable.version = QStringLiteral("1.1.0");
    auto d = decideInstallOrUpdate(s, stubPinned());
    QCOMPARE(d.variant, FlowVariant::BlockingError);
    QCOMPARE(d.blockingReason, BlockingReason::RmPresentNoXovi);
}

void TestRouter::unsupportedModelIsBlockingError() {
    DeviceState s = freshChiappa();
    s.deviceModel = QStringLiteral("reMarkable 2");
    auto d = decideInstallOrUpdate(s, stubPinned());
    QCOMPARE(d.variant, FlowVariant::BlockingError);
    QCOMPARE(d.blockingReason, BlockingReason::UnsupportedModel);
}

void TestRouter::offersBootPersistenceWhenMissing() {
    DeviceState s = freshChiappa();
    s.xovi.installed = true;
    s.xovi.tag = QStringLiteral("v18-23032026");
    s.xovi.bootPersistence = false;  // user installed XOVI without persistence
    s.appload.installed = true;
    s.appload.tag = QStringLiteral("v0.5.0");
    auto d = decideInstallOrUpdate(s, stubPinned());
    QCOMPARE(d.variant, FlowVariant::Install);
    QVERIFY(d.offerBootPersistence);
}

void TestRouter::uninstallShowsRemoveXoviCheckboxOnlyForOurInstall() {
    DeviceState s = freshChiappa();
    s.xovi.installed = true;
    s.xovi.tag = QStringLiteral("v18-23032026");
    s.readmarkable.installed = true;
    s.readmarkable.version = QStringLiteral("1.2.0");

    // Case 1: XOVI installed by us → offer checkbox.
    s.xovi.installSource = QStringLiteral("readmarkable-installer");
    auto d1 = decideUninstall(s);
    QCOMPARE(d1.variant, FlowVariant::Uninstall);
    QVERIFY(d1.showRemoveXoviCheckbox);

    // Case 2: XOVI installed by user → do not show the checkbox.
    s.xovi.installSource = QStringLiteral("user");
    auto d2 = decideUninstall(s);
    QVERIFY(!d2.showRemoveXoviCheckbox);

    // Case 3: XOVI install-source missing → treat as user (no checkbox).
    s.xovi.installSource.clear();
    auto d3 = decideUninstall(s);
    QVERIFY(!d3.showRemoveXoviCheckbox);
}

void TestRouter::updateSummaryStripsLeadingVPrefix() {
    // The on-device VERSION file contains "v1.2.0-dev". Router must not
    // produce "vv1.2.0-dev" in its Update summary (cosmetic bug from the
    // 2026-04-15 real-device test).
    DeviceState s = freshChiappa();
    s.xovi.installed = true;
    s.xovi.tag = QStringLiteral("v18-23032026");
    s.xovi.bootPersistence = true;
    s.appload.installed = true;
    s.readmarkable.installed = true;
    s.readmarkable.version = QStringLiteral("v1.2.0-dev");  // already has leading v
    auto d = decideInstallOrUpdate(s, stubPinned());
    QCOMPARE(d.variant, FlowVariant::Update);
    QVERIFY2(!d.summary.contains(QStringLiteral("vv")),
             qPrintable(d.summary));
    QVERIFY2(d.summary.contains(QStringLiteral("v1.2.0-dev")),
             qPrintable(d.summary));
}

void TestRouter::uninstallSummaryStripsLeadingVPrefix() {
    DeviceState s = freshChiappa();
    s.xovi.installed = true;
    s.readmarkable.installed = true;
    s.readmarkable.version = QStringLiteral("v1.2.0-dev");
    auto d = decideUninstall(s);
    QVERIFY2(!d.summary.contains(QStringLiteral("vv")),
             qPrintable(d.summary));
    QVERIFY2(d.summary.contains(QStringLiteral("v1.2.0-dev")),
             qPrintable(d.summary));
}

QTEST_MAIN(TestRouter)
#include "test_router.moc"
