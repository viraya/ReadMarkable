#include "flow/install_flow.h"

#include <QFileInfo>
#include <QString>
#include <QStringList>

#include "ssh/ssh_client.h"
#include "ssh/ssh_session.h"

namespace rm::installer::flow {

namespace {

QString shellQuote(const QString& s) {
    QString out = s;
    out.replace(QStringLiteral("'"), QStringLiteral("'\\''"));
    return QStringLiteral("'") + out + QStringLiteral("'");
}

Phase mkPhase(const QString& name, bool abortSafe,
              std::function<bool(ssh::SshSession&, QString*)> run) {
    Phase p;
    p.name = name;
    p.abortSafe = abortSafe;
    p.run = std::move(run);
    return p;
}

bool runOk(const ssh::ExecResult& r, QString* err, const QString& label) {
    if (r.timedOut) {
        if (err) *err = label + QStringLiteral(" timed out");
        return false;
    }
    if (r.exitCode != 0) {
        if (err) {
            *err = QStringLiteral("%1 failed (exit %2): %3")
                       .arg(label).arg(r.exitCode)
                       .arg(QString::fromUtf8(r.stdErr.isEmpty() ? r.stdOut
                                                                 : r.stdErr));
        }
        return false;
    }
    return true;
}

bool uploadTo(ssh::SshSession& session, const QString& localPath,
              const QString& remoteDir, int mode, QString* err) {
    const QString base = QFileInfo(localPath).fileName();
    const QString remote = remoteDir + QStringLiteral("/") + base;
    if (!session.sftpPut(localPath, remote, mode, err)) return false;
    return true;
}

Phase probePhase() {
    return mkPhase(
        QStringLiteral("probe"), true,
        [](ssh::SshSession& session, QString* err) -> bool {

            const auto r = session.exec(QStringLiteral("echo ok"));
            return runOk(r, err, QStringLiteral("probe"));
        });
}

Phase cleanupPhase(const QString& stagingDir) {
    return mkPhase(
        QStringLiteral("cleanup"), true,
        [stagingDir](ssh::SshSession& session, QString* err) -> bool {
            const QString cmd =
                QStringLiteral("rm -rf %1").arg(shellQuote(stagingDir));
            const auto r = session.exec(cmd);
            QString localErr;
            if (!runOk(r, &localErr, QStringLiteral("cleanup"))) {
                if (err) *err = localErr;
            }
            return true;
        });
}

Phase installHomeTilePhase(const QString& stagingDir) {
    return mkPhase(
        QStringLiteral("install-home-tile"), true,
        [stagingDir](ssh::SshSession& session, QString* err) -> bool {
            const QString cmd =
                QStringLiteral("sh %1/install-home-tile.sh %1/home-tile")
                    .arg(shellQuote(stagingDir));
            const auto r = session.exec(cmd);
            return runOk(r, err, QStringLiteral("install-home-tile"));
        });
}

Phase rebuildHashtabPhase(const QString& stagingDir) {
    return mkPhase(
        QStringLiteral("rebuild-hashtab"), false,
        [stagingDir](ssh::SshSession& session, QString* err) -> bool {
            const QString cmd =
                QStringLiteral("sh %1/rebuild-hashtab.sh").arg(shellQuote(stagingDir));
            const auto r = session.exec(cmd);
            return runOk(r, err, QStringLiteral("rebuild-hashtab"));
        });
}

Phase restartXochitlPhase() {
    return mkPhase(
        QStringLiteral("restart-xochitl"), true,
        [](ssh::SshSession& session, QString* err) -> bool {
            const QString cmd = QStringLiteral(
                "if [ -x /home/root/xovi/start ]; then "
                "  /home/root/xovi/start; "
                "else "
                "  systemctl restart xochitl; "
                "fi");
            const auto r = session.exec(cmd);
            return runOk(r, err, QStringLiteral("restart-xochitl"));
        });
}

}

QList<Phase> buildInstallFlow(const InstallFlowInputs& in) {
    QList<Phase> phases;
    phases << probePhase();

    phases << mkPhase(
        QStringLiteral("upload-payload"), true,
        [in](ssh::SshSession& session, QString* err) -> bool {
            const auto mkdir = session.exec(QStringLiteral("mkdir -p %1")
                                                 .arg(shellQuote(in.remoteStagingDir)));
            if (!runOk(mkdir, err, QStringLiteral("mkdir staging"))) return false;
            if (!in.skipXoviInstall) {
                if (!uploadTo(session, in.localXoviTarball, in.remoteStagingDir,
                              0644, err)) return false;
            }
            if (!in.skipAppLoadInstall) {
                if (!uploadTo(session, in.localApploadZip, in.remoteStagingDir,
                              0644, err)) return false;
            }
            if (!uploadTo(session, in.localRmTarball, in.remoteStagingDir, 0644,
                          err)) return false;
            for (const QString& f : in.localOnDeviceFiles) {
                const bool isExec = f.endsWith(QStringLiteral(".sh")) ||
                                    f.endsWith(QStringLiteral("xovi-trigger"));
                const int mode = isExec ? 0755 : 0644;
                if (!uploadTo(session, f, in.remoteStagingDir, mode, err)) return false;
            }

            if (!in.localHomeTileFiles.isEmpty()) {
                const QString htDir =
                    in.remoteStagingDir + QStringLiteral("/home-tile");
                const auto mkHt = session.exec(QStringLiteral("mkdir -p %1")
                                                   .arg(shellQuote(htDir)));
                if (!runOk(mkHt, err, QStringLiteral("mkdir home-tile")))
                    return false;
                for (const QString& f : in.localHomeTileFiles) {
                    if (!uploadTo(session, f, htDir, 0644, err)) return false;
                }
            }
            return true;
        });

    if (!in.skipXoviInstall) {
        phases << mkPhase(
            QStringLiteral("install-xovi"), false,
            [in](ssh::SshSession& session, QString* err) -> bool {
                const QString cmd = QStringLiteral(
                    "sh %1/install-xovi.sh %1/xovi-aarch64.tar.gz %2 readmarkable-installer")
                    .arg(shellQuote(in.remoteStagingDir),
                         shellQuote(in.xoviTag));
                return runOk(session.exec(cmd), err,
                             QStringLiteral("install-xovi"));
            });
    }

    if (!in.skipAppLoadInstall) {
        phases << mkPhase(
            QStringLiteral("install-appload"), false,
            [in](ssh::SshSession& session, QString* err) -> bool {
                const QString cmd = QStringLiteral(
                    "sh %1/install-appload.sh %1/appload-aarch64.zip %2 readmarkable-installer")
                    .arg(shellQuote(in.remoteStagingDir),
                         shellQuote(in.apploadTag));
                return runOk(session.exec(cmd), err,
                             QStringLiteral("install-appload"));
            });
    }

    if (!in.localHomeTileFiles.isEmpty()) {
        phases << installHomeTilePhase(in.remoteStagingDir);
    }

    phases << rebuildHashtabPhase(in.remoteStagingDir);

    if (in.installPersistence) {
        phases << mkPhase(
            QStringLiteral("install-trigger"), false,
            [in](ssh::SshSession& session, QString* err) -> bool {

                const QString cmd = QStringLiteral("sh %1/install-trigger.sh %1")
                                        .arg(shellQuote(in.remoteStagingDir));
                return runOk(session.exec(cmd), err,
                             QStringLiteral("install-trigger"));
            });
    }

    phases << mkPhase(
        QStringLiteral("install-readmarkable"), false,
        [in](ssh::SshSession& session, QString* err) -> bool {
            const QString tarballBase = QFileInfo(in.localRmTarball).fileName();

            const QString extractDir =
                in.remoteStagingDir + QStringLiteral("/rm-extract");
            const QString cmd = QStringLiteral(
                "set -e; "
                "mkdir -p %1 && "
                "tar -xzf %2/%3 -C %1 && "
                "cd %1/$(ls %1) && "
                "sh %2/install-readmarkable.sh")
                .arg(shellQuote(extractDir),
                     shellQuote(in.remoteStagingDir),
                     shellQuote(tarballBase));
            return runOk(session.exec(cmd), err,
                         QStringLiteral("install-readmarkable"));
        });

    phases << restartXochitlPhase();
    phases << cleanupPhase(in.remoteStagingDir);

    return phases;
}

QList<Phase> buildUpdateFlow(const UpdateFlowInputs& in) {
    QList<Phase> phases;
    phases << probePhase();

    phases << mkPhase(
        QStringLiteral("upload-payload"), true,
        [in](ssh::SshSession& session, QString* err) -> bool {
            const auto mkdir = session.exec(
                QStringLiteral("mkdir -p %1").arg(shellQuote(in.remoteStagingDir)));
            if (!runOk(mkdir, err, QStringLiteral("mkdir staging"))) return false;
            if (!uploadTo(session, in.localRmTarball, in.remoteStagingDir, 0644,
                          err)) return false;
            if (!uploadTo(session, in.installRmScript, in.remoteStagingDir,
                          0755, err)) return false;
            if (!in.rebuildHashtabScript.isEmpty()) {
                if (!uploadTo(session, in.rebuildHashtabScript,
                              in.remoteStagingDir, 0755, err))
                    return false;
            }
            if (!in.installHomeTileScript.isEmpty() &&
                !in.homeTileFiles.isEmpty()) {
                if (!uploadTo(session, in.installHomeTileScript,
                              in.remoteStagingDir, 0755, err))
                    return false;
                const QString htDir =
                    in.remoteStagingDir + QStringLiteral("/home-tile");
                const auto mkHt = session.exec(QStringLiteral("mkdir -p %1")
                                                   .arg(shellQuote(htDir)));
                if (!runOk(mkHt, err, QStringLiteral("mkdir home-tile")))
                    return false;
                for (const QString& f : in.homeTileFiles) {
                    if (!uploadTo(session, f, htDir, 0644, err)) return false;
                }
            }
            if (in.installPersistence) {
                for (const QString& f : in.persistenceFiles) {
                    const bool isExec = f.endsWith(QStringLiteral(".sh")) ||
                                        f.endsWith(QStringLiteral("xovi-trigger"));
                    const int mode = isExec ? 0755 : 0644;
                    if (!uploadTo(session, f, in.remoteStagingDir, mode, err))
                        return false;
                }
            }
            return true;
        });

    phases << mkPhase(
        QStringLiteral("stop-readmarkable"), true,
        [](ssh::SshSession& session, QString* err) -> bool {

            session.exec(QStringLiteral("killall -q readmarkable; true"));
            (void)err;
            return true;
        });

    phases << mkPhase(
        QStringLiteral("install-readmarkable"), false,
        [in](ssh::SshSession& session, QString* err) -> bool {
            const QString tarballBase = QFileInfo(in.localRmTarball).fileName();
            const QString extractDir =
                in.remoteStagingDir + QStringLiteral("/rm-extract");
            const QString cmd = QStringLiteral(
                "set -e; "
                "mkdir -p %1 && "
                "tar -xzf %2/%3 -C %1 && "
                "cd %1/$(ls %1) && "
                "sh %2/install-readmarkable.sh")
                .arg(shellQuote(extractDir),
                     shellQuote(in.remoteStagingDir),
                     shellQuote(tarballBase));
            return runOk(session.exec(cmd), err,
                         QStringLiteral("install-readmarkable"));
        });

    if (!in.installHomeTileScript.isEmpty() && !in.homeTileFiles.isEmpty()) {
        phases << installHomeTilePhase(in.remoteStagingDir);
    }

    phases << rebuildHashtabPhase(in.remoteStagingDir);

    if (in.installPersistence) {
        phases << mkPhase(
            QStringLiteral("install-trigger"), false,
            [in](ssh::SshSession& session, QString* err) -> bool {
                const QString cmd = QStringLiteral("sh %1/install-trigger.sh %1")
                                        .arg(shellQuote(in.remoteStagingDir));
                return runOk(session.exec(cmd), err,
                             QStringLiteral("install-trigger"));
            });
    }

    phases << restartXochitlPhase();
    phases << cleanupPhase(in.remoteStagingDir);

    return phases;
}

QList<Phase> buildUninstallFlow(const UninstallFlowInputs& in) {
    QList<Phase> phases;
    phases << probePhase();

    phases << mkPhase(
        QStringLiteral("stop-readmarkable"), true,
        [](ssh::SshSession& session, QString* err) -> bool {
            session.exec(QStringLiteral("killall -q readmarkable; true"));
            (void)err;
            return true;
        });

    phases << mkPhase(
        QStringLiteral("xovi-stock"), true,
        [](ssh::SshSession& session, QString* err) -> bool {

            session.exec(QStringLiteral(
                "[ -x /home/root/xovi/stock ] && /home/root/xovi/stock || true"));
            (void)err;
            return true;
        });

    phases << mkPhase(
        QStringLiteral("upload-uninstall-script"), true,
        [in](ssh::SshSession& session, QString* err) -> bool {
            session.exec(QStringLiteral("mkdir -p %1")
                             .arg(shellQuote(in.remoteStagingDir)));
            for (const QString& f : in.localOnDeviceFiles) {
                const bool isExec = f.endsWith(QStringLiteral(".sh")) ||
                                    f.endsWith(QStringLiteral("xovi-trigger"));
                const int mode = isExec ? 0755 : 0644;
                if (!uploadTo(session, f, in.remoteStagingDir, mode, err)) return false;
            }
            return true;
        });

    phases << mkPhase(
        QStringLiteral("uninstall-readmarkable"), false,
        [in](ssh::SshSession& session, QString* err) -> bool {

            const QString cmd = QStringLiteral("sh %1/uninstall.sh --purge")
                                    .arg(shellQuote(in.remoteStagingDir));
            return runOk(session.exec(cmd), err,
                         QStringLiteral("uninstall-readmarkable"));
        });

    phases << mkPhase(
        QStringLiteral("remove-persistence"), false,
        [in](ssh::SshSession& session, QString* err) -> bool {
            const QString cmd =
                QStringLiteral("sh %1/uninstall.sh --remove-persistence")
                    .arg(shellQuote(in.remoteStagingDir));
            return runOk(session.exec(cmd), err,
                         QStringLiteral("remove-persistence"));
        });

    if (in.alsoRemoveXovi) {
        phases << mkPhase(
            QStringLiteral("uninstall-xovi"), false,
            [in](ssh::SshSession& session, QString* err) -> bool {
                const QString cmd =
                    QStringLiteral("sh %1/uninstall.sh --remove-xovi")
                        .arg(shellQuote(in.remoteStagingDir));
                return runOk(session.exec(cmd), err,
                             QStringLiteral("uninstall-xovi"));
            });
    }

    phases << mkPhase(
        QStringLiteral("remove-ssh-key"), true,
        [](ssh::SshSession& session, QString* err) -> bool {

            session.exec(QStringLiteral(
                "[ -f /home/root/.ssh/authorized_keys ] && "
                "sed -i '/readmarkable-installer@/d' /home/root/.ssh/authorized_keys || true"));
            (void)err;
            return true;
        });

    phases << mkPhase(
        QStringLiteral("ensure-stock-xochitl"), true,
        [](ssh::SshSession& session, QString* err) -> bool {
            session.exec(QStringLiteral(
                "[ -x /home/root/xovi/stock ] && /home/root/xovi/stock || "
                "systemctl restart xochitl || true"));
            (void)err;
            return true;
        });

    phases << cleanupPhase(in.remoteStagingDir);

    return phases;
}

}
