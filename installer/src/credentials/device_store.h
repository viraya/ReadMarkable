#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <optional>

namespace rm::installer::credentials {

struct DeviceRecord {
    QString fingerprint;
    QString lastIp;
    QString lastModel;
    QString lastFirmware;
    QDateTime lastConnected;
    QString keyComment;

    QString privateKeyPath;
    QString publicKeyPath;
};

class DeviceStore {
 public:

    DeviceStore();
    explicit DeviceStore(const QString& rootDir);

    std::optional<DeviceRecord> findByFingerprint(const QString& fingerprint) const;

    bool add(const DeviceRecord& record, QString* err = nullptr) const;

    bool remove(const QString& fingerprint, QString* err = nullptr) const;

    QList<DeviceRecord> all() const;

    QString rootDir() const { return rootDir_; }
    QString deviceDirFor(const QString& fingerprint) const;
    QString privateKeyPathFor(const QString& fingerprint) const;
    QString publicKeyPathFor(const QString& fingerprint) const;

 private:
    QString rootDir_;
};

}
