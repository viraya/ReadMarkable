#include "diagnostics/diagnostics_zip.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTemporaryDir>

#include "core/version.h"

namespace rm::installer::diagnostics {

namespace {

void dump(const QString& path, const QString& content) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(content.toUtf8());
}

QString scrubFingerprints(QString text) {

    static const QRegularExpression re(
        QStringLiteral("SHA256:[A-Za-z0-9+/=]{20,}"));
    return text.replace(re, QStringLiteral("SHA256:<scrubbed>"));
}

}

bool writeDiagnosticsArchive(const QString& sessionLogPath,
                             const QString& lastDeviceStateJson,
                             const QString& destArchivePath, QString* err) {
    QTemporaryDir stage;
    if (!stage.isValid()) {
        if (err) *err = QStringLiteral("cannot create staging tempdir");
        return false;
    }
    const QString stageDir = stage.path();

    if (!sessionLogPath.isEmpty() && QFile::exists(sessionLogPath)) {
        QFile src(sessionLogPath);
        if (src.open(QIODevice::ReadOnly)) {
            const QString raw = QString::fromUtf8(src.readAll());
            dump(stageDir + QStringLiteral("/session.log"), scrubFingerprints(raw));
        }
    }

    QFile pins(QStringLiteral(":/config/pinned-versions.toml"));
    if (pins.open(QIODevice::ReadOnly)) {
        dump(stageDir + QStringLiteral("/pinned-versions.toml"),
             QString::fromUtf8(pins.readAll()));
    }

    dump(stageDir + QStringLiteral("/installer-version.txt"),
         QString::fromUtf8(rm::installer::kAppVersion) + QStringLiteral("\n"));

    const QString host = QStringLiteral(
                             "os_name: %1\nos_version: %2\narch: %3\n"
                             "kernel: %4\nlocale: %5\n")
                             .arg(QSysInfo::prettyProductName(),
                                  QSysInfo::productVersion(),
                                  QSysInfo::currentCpuArchitecture(),
                                  QSysInfo::kernelVersion(),
                                  QLocale().name());
    dump(stageDir + QStringLiteral("/host-info.txt"), host);

    if (!lastDeviceStateJson.isEmpty()) {
        dump(stageDir + QStringLiteral("/device-state.json"),
             scrubFingerprints(lastDeviceStateJson));
    }

    QProcess tar;
    tar.setWorkingDirectory(stageDir);
    QStringList args;
    args << QStringLiteral("-czf") << destArchivePath << QStringLiteral(".");
    tar.start(QStringLiteral("tar"), args);
    if (!tar.waitForStarted(5000)) {
        if (err) *err = QStringLiteral("cannot start 'tar': %1")
                            .arg(tar.errorString());
        return false;
    }
    if (!tar.waitForFinished(60000)) {
        if (err) *err = QStringLiteral("tar timed out");
        return false;
    }
    if (tar.exitCode() != 0) {
        if (err) *err = QStringLiteral("tar failed (exit %1): %2")
                            .arg(tar.exitCode())
                            .arg(QString::fromLocal8Bit(tar.readAllStandardError()));
        return false;
    }
    return true;
}

}
