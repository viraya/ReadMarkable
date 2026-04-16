#include "credentials/device_store.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>

#define TOML_EXCEPTIONS 0
#include <tomlplusplus/toml.hpp>

#include <sstream>
#include <string>

namespace rm::installer::credentials {

namespace {

QString sanitizeFingerprint(const QString& fp) {
    QString out = fp;
    out.replace(QLatin1Char('/'), QLatin1Char('_'));
    out.replace(QLatin1Char('='), QLatin1Char('_'));
    out.replace(QLatin1Char(':'), QLatin1Char('_'));
    return out;
}

QString defaultRoot() {
    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (dir.isEmpty()) return {};
    return dir + QStringLiteral("/devices");
}

bool writeMetaToml(const QString& path, const DeviceRecord& rec, QString* err) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (err) *err = QStringLiteral("cannot write %1").arg(path);
        return false;
    }
    std::ostringstream oss;
    oss << "# readmarkable-installer device metadata\n";
    oss << "fingerprint = \"" << rec.fingerprint.toStdString() << "\"\n";
    oss << "last_ip = \"" << rec.lastIp.toStdString() << "\"\n";
    oss << "last_model = \"" << rec.lastModel.toStdString() << "\"\n";
    oss << "last_firmware = \"" << rec.lastFirmware.toStdString() << "\"\n";
    oss << "last_connected = \""
        << rec.lastConnected.toUTC()
               .toString(Qt::ISODateWithMs)
               .toStdString()
        << "\"\n";
    oss << "key_comment = \"" << rec.keyComment.toStdString() << "\"\n";
    const std::string text = oss.str();
    f.write(text.c_str(), static_cast<qint64>(text.size()));
    if (!f.commit()) {
        if (err) *err = QStringLiteral("commit failed: %1").arg(path);
        return false;
    }
    return true;
}

std::optional<DeviceRecord> readMetaToml(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    const QByteArray bytes = f.readAll();
    auto result = toml::parse(std::string_view(bytes.constData(),
                                                 static_cast<size_t>(bytes.size())));
    if (!result) return std::nullopt;
    const toml::table& root = result.table();
    DeviceRecord r;
    if (auto s = root["fingerprint"].as_string())
        r.fingerprint = QString::fromStdString(s->get());
    if (auto s = root["last_ip"].as_string())
        r.lastIp = QString::fromStdString(s->get());
    if (auto s = root["last_model"].as_string())
        r.lastModel = QString::fromStdString(s->get());
    if (auto s = root["last_firmware"].as_string())
        r.lastFirmware = QString::fromStdString(s->get());
    if (auto s = root["last_connected"].as_string()) {
        r.lastConnected = QDateTime::fromString(
            QString::fromStdString(s->get()), Qt::ISODateWithMs);
    }
    if (auto s = root["key_comment"].as_string())
        r.keyComment = QString::fromStdString(s->get());
    if (r.fingerprint.isEmpty()) return std::nullopt;
    return r;
}

}

DeviceStore::DeviceStore() : rootDir_(defaultRoot()) {}
DeviceStore::DeviceStore(const QString& rootDir) : rootDir_(rootDir) {}

QString DeviceStore::deviceDirFor(const QString& fingerprint) const {
    return rootDir_ + QLatin1Char('/') + sanitizeFingerprint(fingerprint);
}

QString DeviceStore::privateKeyPathFor(const QString& fingerprint) const {
    return deviceDirFor(fingerprint) + QStringLiteral("/id_ed25519");
}

QString DeviceStore::publicKeyPathFor(const QString& fingerprint) const {
    return deviceDirFor(fingerprint) + QStringLiteral("/id_ed25519.pub");
}

std::optional<DeviceRecord> DeviceStore::findByFingerprint(
    const QString& fingerprint) const {
    const QString dir = deviceDirFor(fingerprint);
    if (!QFileInfo(dir).isDir()) return std::nullopt;
    auto rec = readMetaToml(dir + QStringLiteral("/meta.toml"));
    if (!rec) return std::nullopt;
    rec->privateKeyPath = privateKeyPathFor(fingerprint);
    rec->publicKeyPath = publicKeyPathFor(fingerprint);
    return rec;
}

bool DeviceStore::add(const DeviceRecord& record, QString* err) const {
    if (record.fingerprint.isEmpty()) {
        if (err) *err = QStringLiteral("fingerprint required");
        return false;
    }
    const QString dir = deviceDirFor(record.fingerprint);
    QDir().mkpath(dir);
    return writeMetaToml(dir + QStringLiteral("/meta.toml"), record, err);
}

bool DeviceStore::remove(const QString& fingerprint, QString* err) const {
    const QString dir = deviceDirFor(fingerprint);
    QDir d(dir);
    if (!d.exists()) return false;
    if (!d.removeRecursively()) {
        if (err) *err = QStringLiteral("removeRecursively failed: %1").arg(dir);
        return false;
    }
    return true;
}

QList<DeviceRecord> DeviceStore::all() const {
    QList<DeviceRecord> out;
    QDir root(rootDir_);
    if (!root.exists()) return out;
    for (const QFileInfo& info : root.entryInfoList(
             QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        auto rec = readMetaToml(info.absoluteFilePath() + QStringLiteral("/meta.toml"));
        if (!rec) continue;
        rec->privateKeyPath = info.absoluteFilePath() + QStringLiteral("/id_ed25519");
        rec->publicKeyPath = info.absoluteFilePath() + QStringLiteral("/id_ed25519.pub");
        out << *rec;
    }
    return out;
}

}
