#pragma once

#include <QString>
#include <memory>

#include "credentials/device_store.h"
#include "ssh/known_hosts.h"
#include "ssh/ssh_session.h"

namespace rm::installer::credentials {

class AuthHandler {
 public:
    AuthHandler(DeviceStore* store, ssh::KnownHosts* knownHosts);

    struct Request {
        QString host;
        quint16 port = 22;
        QString user = QStringLiteral("root");
        QString password;
        bool rememberDevice = false;
        int connectTimeoutMs = 10000;
    };

    enum class Status {
        Ok,
        ConnectionFailed,
        FingerprintMismatch,
        AuthenticationFailed,
        KeyInstallFailed,
        StoreWriteFailed,
    };

    struct Result {
        Status status = Status::ConnectionFailed;
        QString err;

        std::unique_ptr<ssh::SshSession> session;
        QString fingerprint;
        bool usedStoredKey = false;
        bool newKeyInstalled = false;

        QString expectedFingerprint;
        QString actualFingerprint;
    };

    Result tryConnect(const Request& req);

 private:
    DeviceStore* store_;
    ssh::KnownHosts* knownHosts_;
};

}
