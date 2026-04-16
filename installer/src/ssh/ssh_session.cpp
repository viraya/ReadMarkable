#include "ssh/ssh_session.h"

#include <QElapsedTimer>
#include <QThread>
#include <atomic>

namespace rm::installer::ssh {

struct SshSession::Impl {
    Config cfg;
    std::unique_ptr<SshClient> client;
    bool open = false;
    std::atomic<bool> aborting{false};
};

SshSession::SshSession(QObject* parent) : QObject(parent), d_(std::make_unique<Impl>()) {}
SshSession::~SshSession() = default;

void SshSession::wireClientSignals() {
    if (!d_->client) return;
    connect(d_->client.get(), &SshClient::lineReceived, this,
            &SshSession::lineReceived);
}

bool SshSession::open(const Config& cfg, QString* err) {
    close();
    d_->cfg = cfg;
    return reconnect(err);
}

bool SshSession::reconnect(QString* err) {
    d_->client = std::make_unique<SshClient>();
    wireClientSignals();
    if (!d_->client->connectToHost(d_->cfg.host, d_->cfg.port,
                                    d_->cfg.connectTimeoutMs, err)) {
        d_->client.reset();
        return false;
    }
    bool authOk = false;
    if (!d_->cfg.privateKeyPath.isEmpty()) {
        authOk = d_->client->authenticatePrivateKeyFile(
            d_->cfg.user, d_->cfg.privateKeyPath, d_->cfg.passphrase, err);
    } else {
        authOk = d_->client->authenticatePassword(d_->cfg.user, d_->cfg.password,
                                                   err);
    }
    if (!authOk) {
        d_->client.reset();
        return false;
    }
    d_->open = true;
    return true;
}

void SshSession::close() {
    if (d_->client) {
        d_->client->disconnectFromHost();
        d_->client.reset();
    }
    d_->open = false;
}

bool SshSession::isOpen() const { return d_->open && d_->client != nullptr; }

QString SshSession::serverFingerprint() const {
    return d_->client ? d_->client->serverFingerprint() : QString();
}

ExecResult SshSession::exec(const QString& command) {
    return exec(command, d_->cfg.commandTimeoutMs);
}

ExecResult SshSession::exec(const QString& command, int timeoutMs) {
    int backoff = d_->cfg.initialBackoffMs;
    for (int attempt = 1; attempt <= std::max(1, d_->cfg.maxRetries); ++attempt) {
        if (!isOpen()) {
            emit reconnecting(attempt, d_->cfg.maxRetries);
            if (!reconnect(nullptr)) {
                QThread::msleep(backoff);
                backoff *= 2;
                continue;
            }
        }
        ExecResult r = d_->client->exec(command, timeoutMs);

        if (r.timedOut || (!d_->client->isConnected())) {
            close();
            if (attempt == d_->cfg.maxRetries) return r;
            emit reconnecting(attempt + 1, d_->cfg.maxRetries);
            QThread::msleep(backoff);
            backoff *= 2;
            continue;
        }
        return r;
    }
    ExecResult r;
    r.exitCode = -1;
    return r;
}

bool SshSession::sftpPut(const QString& localPath, const QString& remotePath,
                         int remoteMode, QString* err) {
    int backoff = d_->cfg.initialBackoffMs;
    for (int attempt = 1; attempt <= std::max(1, d_->cfg.maxRetries); ++attempt) {
        if (!isOpen()) {
            emit reconnecting(attempt, d_->cfg.maxRetries);
            if (!reconnect(err)) {
                QThread::msleep(backoff);
                backoff *= 2;
                continue;
            }
        }
        if (d_->client->sftpPut(localPath, remotePath, remoteMode,
                                d_->cfg.commandTimeoutMs, err)) {
            return true;
        }

        close();
        if (attempt == d_->cfg.maxRetries) return false;
        emit reconnecting(attempt + 1, d_->cfg.maxRetries);
        QThread::msleep(backoff);
        backoff *= 2;
    }
    return false;
}

void SshSession::abort() {
    d_->aborting = true;
    if (d_->client) d_->client->abort();
}

}
