#include "ssh/known_hosts.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>
#include <QStringList>

namespace rm::installer::ssh {

namespace {

QString defaultKnownHostsPath() {
    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (dir.isEmpty()) {
        return QString();
    }
    QDir().mkpath(dir);
    return dir + QStringLiteral("/known_hosts");
}

}

KnownHosts::KnownHosts() : filePath_(defaultKnownHostsPath()) {}
KnownHosts::KnownHosts(const QString& filePath) : filePath_(filePath) {}

bool KnownHosts::ensureLoaded() {
    if (loaded_) return true;
    loaded_ = true;
    if (filePath_.isEmpty()) return true;
    QFile f(filePath_);
    if (!f.exists()) return true;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    while (!f.atEnd()) {
        const QByteArray raw = f.readLine().trimmed();
        if (raw.isEmpty() || raw.startsWith('#')) continue;

        const int tab = raw.indexOf('\t');
        if (tab <= 0) continue;
        const QString host = QString::fromUtf8(raw.left(tab)).toLower();
        const QString fp = QString::fromUtf8(raw.mid(tab + 1));
        if (!host.isEmpty() && !fp.isEmpty()) {
            cache_.insert(host, fp);
        }
    }
    return true;
}

bool KnownHosts::save() {
    if (filePath_.isEmpty()) return false;
    QDir().mkpath(QFileInfo(filePath_).absolutePath());
    QSaveFile f(filePath_);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    f.write("# ReadMarkable installer, TOFU host-key store\n");
    f.write("# host<TAB>fingerprint (SHA256:…)\n");
    for (auto it = cache_.constBegin(); it != cache_.constEnd(); ++it) {
        f.write(it.key().toUtf8());
        f.write("\t");
        f.write(it.value().toUtf8());
        f.write("\n");
    }
    return f.commit();
}

KnownHostsStatus KnownHosts::verify(const QString& host,
                                    const QString& fingerprint) {
    ensureLoaded();
    const QString key = host.toLower();
    auto it = cache_.constFind(key);
    if (it == cache_.constEnd()) return KnownHostsStatus::NewHost;
    return it.value() == fingerprint ? KnownHostsStatus::Match
                                     : KnownHostsStatus::Mismatch;
}

bool KnownHosts::add(const QString& host, const QString& fingerprint) {
    ensureLoaded();
    cache_.insert(host.toLower(), fingerprint);
    return save();
}

bool KnownHosts::remove(const QString& host) {
    ensureLoaded();
    const int n = cache_.remove(host.toLower());
    if (n == 0) return false;
    return save();
}

int KnownHosts::size() {
    ensureLoaded();
    return cache_.size();
}

}
