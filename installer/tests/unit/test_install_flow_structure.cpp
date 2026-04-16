// Structure-only test: validates that buildInstall/Update/UninstallFlow()
// produce the phase sequences required by spec §6, respecting the
// conditional branches (skipXoviInstall, skipAppLoadInstall, alsoRemoveXovi).
// Does NOT execute any phase, no SSH required.

#include <QString>
#include <QtTest/QtTest>

#include "flow/install_flow.h"

using rm::installer::flow::InstallFlowInputs;
using rm::installer::flow::Phase;
using rm::installer::flow::UninstallFlowInputs;
using rm::installer::flow::UpdateFlowInputs;
using rm::installer::flow::buildInstallFlow;
using rm::installer::flow::buildUninstallFlow;
using rm::installer::flow::buildUpdateFlow;

class TestInstallFlowStructure : public QObject {
    Q_OBJECT
 private slots:
    void freshInstallHasAllTenPhases();
    void skipXoviRemovesInstallXoviPhase();
    void skipAppLoadRemovesInstallApploadPhase();
    void skipAppLoadStillHasRebuildHashtab();
    void skipPersistenceRemovesInstallPersistencePhase();
    void emptyHomeTileFilesSkipsHomeTilePhase();
    void updateFlowHasEightPhases();
    void updateFlowWithPersistenceAddsInstallPersistence();
    void updateFlowWithoutHomeTileSkipsPhase();
    void uninstallFlowDefaultEightPhases();
    void uninstallAlsoRemoveXoviAddsPhase();
};

namespace {

QStringList phaseNames(const QList<Phase>& phases) {
    QStringList names;
    for (const Phase& p : phases) names << p.name;
    return names;
}

InstallFlowInputs freshInputs() {
    InstallFlowInputs in;
    in.remoteStagingDir = QStringLiteral("/tmp/rm-install-abc");
    in.localXoviTarball = QStringLiteral("/tmp/xovi.tar.gz");
    in.localApploadZip = QStringLiteral("/tmp/appload.zip");
    in.localRmTarball = QStringLiteral("/tmp/rm.tar.gz");
    in.localHomeTileFiles = {
        QStringLiteral("/tmp/manifest.toml"),
        QStringLiteral("/tmp/readmarkable-icon.rcc"),
        QStringLiteral("/tmp/readmarkable-3.26.0.68.qmd"),
    };
    in.xoviTag = QStringLiteral("v18-23032026");
    in.apploadTag = QStringLiteral("v0.5.0");
    in.installPersistence = true;
    return in;
}

}  // namespace

void TestInstallFlowStructure::freshInstallHasAllTenPhases() {
    auto phases = buildInstallFlow(freshInputs());
    QCOMPARE(phaseNames(phases),
             (QStringList{"probe", "upload-payload", "install-xovi",
                          "install-appload", "install-home-tile",
                          "rebuild-hashtab", "install-trigger",
                          "install-readmarkable", "restart-xochitl",
                          "cleanup"}));
}

void TestInstallFlowStructure::emptyHomeTileFilesSkipsHomeTilePhase() {
    // When no home-tile payload is bundled (e.g. a build explicitly stripped
    // it), the flow should gracefully omit the phase rather than attempt
    // an upload of zero files.
    auto in = freshInputs();
    in.localHomeTileFiles.clear();
    auto names = phaseNames(buildInstallFlow(in));
    QVERIFY(!names.contains(QStringLiteral("install-home-tile")));
    QVERIFY(names.contains(QStringLiteral("rebuild-hashtab")));
}

void TestInstallFlowStructure::skipAppLoadStillHasRebuildHashtab() {
    // Rationale: skipAppLoadInstall=true means XOVI is already present with
    // AppLoad (and thus qt-resource-rebuilder). A firmware OTA since the
    // user's last Install may have invalidated the hashtab, so we still
    // regenerate. The script's internal mtime check makes the no-op path
    // fast on unchanged firmware.
    auto in = freshInputs();
    in.skipAppLoadInstall = true;
    auto names = phaseNames(buildInstallFlow(in));
    QVERIFY(names.contains(QStringLiteral("rebuild-hashtab")));
}

void TestInstallFlowStructure::skipXoviRemovesInstallXoviPhase() {
    auto in = freshInputs();
    in.skipXoviInstall = true;
    auto names = phaseNames(buildInstallFlow(in));
    QVERIFY(!names.contains(QStringLiteral("install-xovi")));
    QVERIFY(names.contains(QStringLiteral("install-appload")));
}

void TestInstallFlowStructure::skipAppLoadRemovesInstallApploadPhase() {
    auto in = freshInputs();
    in.skipAppLoadInstall = true;
    auto names = phaseNames(buildInstallFlow(in));
    QVERIFY(!names.contains(QStringLiteral("install-appload")));
}

void TestInstallFlowStructure::skipPersistenceRemovesInstallPersistencePhase() {
    auto in = freshInputs();
    in.installPersistence = false;
    auto names = phaseNames(buildInstallFlow(in));
    QVERIFY(!names.contains(QStringLiteral("install-trigger")));
}

void TestInstallFlowStructure::updateFlowHasEightPhases() {
    UpdateFlowInputs in;
    in.remoteStagingDir = QStringLiteral("/tmp/rm-update");
    in.localRmTarball = QStringLiteral("/tmp/rm.tar.gz");
    in.installRmScript = QStringLiteral("/tmp/install-readmarkable.sh");
    in.rebuildHashtabScript = QStringLiteral("/tmp/rebuild-hashtab.sh");
    in.installHomeTileScript = QStringLiteral("/tmp/install-home-tile.sh");
    in.homeTileFiles = {
        QStringLiteral("/tmp/manifest.toml"),
        QStringLiteral("/tmp/readmarkable-icon.rcc"),
        QStringLiteral("/tmp/readmarkable-3.26.0.68.qmd"),
    };
    QCOMPARE(phaseNames(buildUpdateFlow(in)),
             (QStringList{"probe", "upload-payload", "stop-readmarkable",
                          "install-readmarkable", "install-home-tile",
                          "rebuild-hashtab", "restart-xochitl", "cleanup"}));
}

void TestInstallFlowStructure::updateFlowWithoutHomeTileSkipsPhase() {
    UpdateFlowInputs in;
    in.remoteStagingDir = QStringLiteral("/tmp/rm-update");
    in.localRmTarball = QStringLiteral("/tmp/rm.tar.gz");
    in.installRmScript = QStringLiteral("/tmp/install-readmarkable.sh");
    in.rebuildHashtabScript = QStringLiteral("/tmp/rebuild-hashtab.sh");
    // homeTileFiles + installHomeTileScript left empty
    auto names = phaseNames(buildUpdateFlow(in));
    QVERIFY(!names.contains(QStringLiteral("install-home-tile")));
}

void TestInstallFlowStructure::updateFlowWithPersistenceAddsInstallPersistence() {
    UpdateFlowInputs in;
    in.remoteStagingDir = QStringLiteral("/tmp/rm-update");
    in.localRmTarball = QStringLiteral("/tmp/rm.tar.gz");
    in.installRmScript = QStringLiteral("/tmp/install-readmarkable.sh");
    in.rebuildHashtabScript = QStringLiteral("/tmp/rebuild-hashtab.sh");
    in.installHomeTileScript = QStringLiteral("/tmp/install-home-tile.sh");
    in.homeTileFiles = {
        QStringLiteral("/tmp/manifest.toml"),
        QStringLiteral("/tmp/readmarkable-icon.rcc"),
        QStringLiteral("/tmp/readmarkable-3.26.0.68.qmd"),
    };
    in.installPersistence = true;
    in.persistenceFiles = {
        QStringLiteral("/tmp/install-trigger.sh"),
        QStringLiteral("/tmp/xovi-trigger.service"),
        QStringLiteral("/tmp/xovi-trigger"),
    };
    QCOMPARE(phaseNames(buildUpdateFlow(in)),
             (QStringList{"probe", "upload-payload", "stop-readmarkable",
                          "install-readmarkable", "install-home-tile",
                          "rebuild-hashtab", "install-trigger",
                          "restart-xochitl", "cleanup"}));
}

void TestInstallFlowStructure::uninstallFlowDefaultEightPhases() {
    UninstallFlowInputs in;
    in.remoteStagingDir = QStringLiteral("/tmp/rm-uninstall");
    in.alsoRemoveXovi = false;
    auto names = phaseNames(buildUninstallFlow(in));
    QVERIFY(!names.contains(QStringLiteral("uninstall-xovi")));
    QVERIFY(!names.contains(QStringLiteral("restart-xochitl")));
    QVERIFY(names.contains(QStringLiteral("xovi-stock")));
    QVERIFY(names.contains(QStringLiteral("remove-persistence")));
    QVERIFY(names.contains(QStringLiteral("ensure-stock-xochitl")));
}

void TestInstallFlowStructure::uninstallAlsoRemoveXoviAddsPhase() {
    UninstallFlowInputs in;
    in.remoteStagingDir = QStringLiteral("/tmp/rm-uninstall");
    in.alsoRemoveXovi = true;
    auto names = phaseNames(buildUninstallFlow(in));
    QVERIFY(names.contains(QStringLiteral("uninstall-xovi")));
}

QTEST_MAIN(TestInstallFlowStructure)
#include "test_install_flow_structure.moc"
