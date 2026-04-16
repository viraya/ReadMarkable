#include "credentials/auth_handler.h"

#include <QDateTime>
#include <QHostInfo>

#include "credentials/keypair.h"

namespace rm::installer::credentials {

namespace {

QString commentFor(const QString& host) {
    return QStringLiteral("readmarkable-installer@%1-%2")
        .arg(host, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
}

QString singleQuote(const QString& v) {
    QString out = v;
    out.replace(QStringLiteral("'"), QStringLiteral("'\\''"));
    return QStringLiteral("'") + out + QStringLiteral("'");
}

}

AuthHandler::AuthHandler(DeviceStore* store, ssh::KnownHosts* knownHosts)
    : store_(store), knownHosts_(knownHosts) {}

AuthHandler::Result AuthHandler::tryConnect(const Request& req) {
    Result r;

    auto probe = std::make_unique<ssh::SshClient>();
    QString err;
    if (!probe->connectToHost(req.host, req.port, req.connectTimeoutMs, &err)) {
        r.status = Status::ConnectionFailed;
        r.err = err;
        return r;
    }
    r.fingerprint = probe->serverFingerprint();

    if (knownHosts_) {
        const auto verdict = knownHosts_->verify(req.host, r.fingerprint);
        if (verdict == ssh::KnownHostsStatus::Mismatch) {
            r.status = Status::FingerprintMismatch;
            r.err = QStringLiteral(
                "Stored fingerprint for %1 does not match the server's "
                "presented key, refusing to continue. Delete the entry in "
                "the installer's known_hosts file to re-trust.").arg(req.host);
            return r;
        }
        if (verdict == ssh::KnownHostsStatus::NewHost) {

            knownHosts_->add(req.host, r.fingerprint);
        }
    }
    probe.reset();

    std::optional<DeviceRecord> stored;
    if (store_) stored = store_->findByFingerprint(r.fingerprint);

    auto session = std::make_unique<ssh::SshSession>();
    ssh::SshSession::Config cfg;
    cfg.host = req.host;
    cfg.port = req.port;
    cfg.user = req.user;
    cfg.connectTimeoutMs = req.connectTimeoutMs;

    if (req.password.isEmpty()) {
        if (!stored) {
            r.status = Status::AuthenticationFailed;
            r.err = QStringLiteral(
                "No saved key for this device and no password provided.");
            return r;
        }
        cfg.privateKeyPath = stored->privateKeyPath;
        if (!session->open(cfg, &err)) {
            r.status = Status::AuthenticationFailed;
            r.err = err;
            return r;
        }
        r.usedStoredKey = true;
        r.session = std::move(session);
        r.status = Status::Ok;
        return r;
    }

    cfg.password = req.password;
    if (!session->open(cfg, &err)) {
        r.status = Status::AuthenticationFailed;
        r.err = err;
        return r;
    }

    if (req.rememberDevice && store_) {
        const QString comment = commentFor(req.host);
        auto kp = Ed25519Keypair::generate(comment, &err);
        if (!kp) {

            r.err = err;
            r.session = std::move(session);
            r.status = Status::Ok;
            return r;
        }
        DeviceRecord rec;
        rec.fingerprint = r.fingerprint;
        rec.lastIp = req.host;
        rec.lastConnected = QDateTime::currentDateTimeUtc();
        rec.keyComment = comment;
        const QString privPath = store_->privateKeyPathFor(r.fingerprint);
        const QString pubPath = store_->publicKeyPathFor(r.fingerprint);

        if (!kp->saveToFiles(privPath, pubPath, &err)) {
            r.status = Status::StoreWriteFailed;
            r.err = err;
            r.session = std::move(session);
            return r;
        }
        if (!store_->add(rec, &err)) {
            r.status = Status::StoreWriteFailed;
            r.err = err;
            r.session = std::move(session);
            return r;
        }

        const QString pubLine = QString::fromUtf8(kp->publicKeyOpenSsh).trimmed();
        const QString cmd = QStringLiteral(
            "mkdir -p /home/root/.ssh && chmod 700 /home/root/.ssh && "
            "touch /home/root/.ssh/authorized_keys && "
            "chmod 600 /home/root/.ssh/authorized_keys && "
            "sed -i '/^%s$/d' /home/root/.ssh/authorized_keys && "
            "printf '%s\\n' %1 >> /home/root/.ssh/authorized_keys")
                              .arg(singleQuote(pubLine));
        const auto exec = session->exec(cmd);
        if (exec.exitCode != 0) {
            r.status = Status::KeyInstallFailed;
            r.err = QStringLiteral("remote authorized_keys install failed: %1")
                        .arg(QString::fromUtf8(exec.stdErr));
            r.session = std::move(session);
            return r;
        }
        r.newKeyInstalled = true;
    }

    r.session = std::move(session);
    r.status = Status::Ok;
    return r;
}

}
