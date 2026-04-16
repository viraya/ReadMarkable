#include "payload/cache.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>
#include <algorithm>

namespace rm::installer::payload {

namespace {

QString defaultCacheDir() {
    const QString base = QStandardPaths::writableLocation(
        QStandardPaths::CacheLocation);
    if (base.isEmpty()) return {};
    return base + QStringLiteral("/releases");
}

}

Cache::Cache() : cacheDir_(defaultCacheDir()) {}
Cache::Cache(const QString& cacheDir) : cacheDir_(cacheDir) {}

void Cache::ensureDir() const {
    if (!cacheDir_.isEmpty()) {
        QDir().mkpath(cacheDir_);
    }
}

QString Cache::tarballPathFor(const QString& version) const {
    return cacheDir_ + QStringLiteral("/readmarkable-") + version +
           QStringLiteral("-chiappa.tar.gz");
}

QString Cache::sha256PathFor(const QString& version) const {
    return tarballPathFor(version) + QStringLiteral(".sha256");
}

std::optional<QString> Cache::lookup(const QString& version) const {
    const QString p = tarballPathFor(version);
    if (QFile::exists(p)) return p;
    return std::nullopt;
}

bool Cache::store(const QString& version, const QString& srcPath, QString* err) {
    ensureDir();
    const QString dst = tarballPathFor(version);
    if (QFile::exists(dst)) {
        QFile::remove(dst);
    }
    if (!QFile::rename(srcPath, dst)) {

        if (!QFile::copy(srcPath, dst)) {
            if (err) *err = QStringLiteral("cannot move %1 → %2")
                                .arg(srcPath, dst);
            return false;
        }
        QFile::remove(srcPath);
    }
    return true;
}

void Cache::evict(int keep) {
    QDir d(cacheDir_);
    if (!d.exists()) return;
    const QFileInfoList entries = d.entryInfoList(
        QStringList{QStringLiteral("readmarkable-*-chiappa.tar.gz")},
        QDir::Files, QDir::Time);
    for (int i = keep; i < entries.size(); ++i) {
        QFile::remove(entries[i].absoluteFilePath());

        QFile::remove(entries[i].absoluteFilePath() + QStringLiteral(".sha256"));
    }
}

}
