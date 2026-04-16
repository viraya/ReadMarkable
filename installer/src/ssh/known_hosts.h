#pragma once

#include <QMap>
#include <QString>

namespace rm::installer::ssh {

enum class KnownHostsStatus {

    NewHost,

    Match,

    Mismatch,
};

class KnownHosts {
 public:

    KnownHosts();
    explicit KnownHosts(const QString& filePath);

    KnownHostsStatus verify(const QString& host, const QString& fingerprint);

    bool add(const QString& host, const QString& fingerprint);

    bool remove(const QString& host);

    QString filePath() const { return filePath_; }
    int size();

 private:
    QString filePath_;
    QMap<QString, QString> cache_;
    bool loaded_ = false;

    bool ensureLoaded();
    bool save();
};

}
