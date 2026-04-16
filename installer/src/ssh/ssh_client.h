#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <memory>

namespace rm::installer::ssh {

struct ExecResult {
    QByteArray stdOut;
    QByteArray stdErr;
    int exitCode = -1;
    bool timedOut = false;
};

enum class Stream { StdOut, StdErr };

class SshClient : public QObject {
    Q_OBJECT
 public:
    explicit SshClient(QObject* parent = nullptr);
    ~SshClient() override;

    bool connectToHost(const QString& host, quint16 port = 22,
                       int timeoutMs = 10000, QString* err = nullptr);
    bool isConnected() const;
    void disconnectFromHost();

    QString serverFingerprint() const;

    bool authenticatePassword(const QString& user, const QString& password,
                              QString* err = nullptr);

    bool authenticatePrivateKeyFile(const QString& user,
                                    const QString& privateKeyPath,
                                    const QString& passphrase = {},
                                    QString* err = nullptr);
    bool isAuthenticated() const;

    ExecResult exec(const QString& command, int timeoutMs = 300000);

    bool sftpPut(const QString& localPath, const QString& remotePath,
                 int remoteMode = 0644, int timeoutMs = 300000,
                 QString* err = nullptr);

    void abort();

 signals:

    void lineReceived(rm::installer::ssh::Stream stream, const QByteArray& line);

 private:
    struct Impl;
    std::unique_ptr<Impl> d_;
};

}
