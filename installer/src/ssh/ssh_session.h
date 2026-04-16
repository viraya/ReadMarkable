#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "ssh/ssh_client.h"

namespace rm::installer::ssh {

class SshSession : public QObject {
    Q_OBJECT
 public:
    struct Config {
        QString host;
        quint16 port = 22;
        QString user;

        QString password;
        QString privateKeyPath;
        QString passphrase;
        int connectTimeoutMs = 10000;
        int commandTimeoutMs = 300000;
        int maxRetries = 3;
        int initialBackoffMs = 500;
    };

    explicit SshSession(QObject* parent = nullptr);
    ~SshSession() override;

    bool open(const Config& cfg, QString* err = nullptr);
    void close();

    bool isOpen() const;
    QString serverFingerprint() const;

    ExecResult exec(const QString& command);
    ExecResult exec(const QString& command, int timeoutMs);

    bool sftpPut(const QString& localPath, const QString& remotePath,
                 int remoteMode = 0644, QString* err = nullptr);

    void abort();

 signals:

    void lineReceived(rm::installer::ssh::Stream stream, const QByteArray& line);

    void reconnecting(int attempt, int maxAttempts);

 private:
    struct Impl;
    std::unique_ptr<Impl> d_;

    bool reconnect(QString* err);
    void wireClientSignals();
};

}
