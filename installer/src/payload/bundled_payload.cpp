#include "payload/bundled_payload.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QResource>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

namespace rm::installer::payload {

namespace {

const QStringList& onDeviceResourceNames() {
    static const QStringList kNames = {
        QStringLiteral("detect-state.sh"),
        QStringLiteral("install-xovi.sh"),
        QStringLiteral("install-appload.sh"),
        QStringLiteral("install-trigger.sh"),
        QStringLiteral("install-readmarkable.sh"),
        QStringLiteral("install-home-tile.sh"),
        QStringLiteral("rebuild-hashtab.sh"),
        QStringLiteral("uninstall.sh"),
        QStringLiteral("xovi-trigger.service"),
        QStringLiteral("xovi-trigger"),
    };
    return kNames;
}

const QStringList& homeTileResourceNames() {
    static const QStringList kNames = {
        QStringLiteral("manifest.toml"),
        QStringLiteral("readmarkable-icon.png"),
        QStringLiteral("readmarkable-3.26.0.68.qmd"),
    };
    return kNames;
}

bool copyResourceToFile(const QString& resourcePath, const QString& destPath,
                        QString* err) {
    QFile in(resourcePath);
    if (!in.open(QIODevice::ReadOnly)) {
        if (err) {
            *err = QStringLiteral("cannot open resource %1").arg(resourcePath);
        }
        return false;
    }
    QFile out(destPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (err) {
            *err = QStringLiteral("cannot write %1").arg(destPath);
        }
        return false;
    }
    constexpr qint64 kChunk = 64 * 1024;
    while (!in.atEnd()) {
        QByteArray buf = in.read(kChunk);
        if (out.write(buf) != buf.size()) {
            if (err) {
                *err = QStringLiteral("short write on %1").arg(destPath);
            }
            return false;
        }
    }
    return true;
}

bool verifySha256File(const QString& destPath, const QString& expectedHex,
                      QString* err) {
    const QByteArray actual = BundledPayload::sha256OfFile(destPath);
    const QByteArray expected = expectedHex.toLower().toUtf8();
    if (actual != expected) {
        if (err) {
            *err = QStringLiteral(
                       "sha256 mismatch for %1\n  expected: %2\n  actual:   %3")
                       .arg(destPath, QString::fromUtf8(expected),
                            QString::fromUtf8(actual));
        }
        return false;
    }
    return true;
}

}

BundledPayload::BundledPayload(const core::PinnedConfig& pinned) : pinned_(pinned) {}

BundledPayload::~BundledPayload() = default;

bool BundledPayload::extractAll(QString* err) {

    work_ = std::make_unique<QTemporaryDir>(
        QDir::tempPath() + QStringLiteral("/rm-installer-payload-XXXXXX"));
    if (!work_->isValid()) {
        if (err) {
            *err = QStringLiteral("cannot create temp dir: %1")
                       .arg(work_->errorString());
        }
        return false;
    }
    paths_.workDir = work_->path();

    paths_.xoviTarball = paths_.workDir + QStringLiteral("/xovi-aarch64.tar.gz");
    if (!copyResourceToFile(QStringLiteral(":/payload/xovi-aarch64.tar.gz"),
                            paths_.xoviTarball, err)) {
        return false;
    }
    if (!verifySha256File(paths_.xoviTarball, pinned_.xovi.sha256, err)) {
        return false;
    }

    paths_.apploadZip = paths_.workDir + QStringLiteral("/appload-aarch64.zip");
    if (!copyResourceToFile(QStringLiteral(":/payload/appload-aarch64.zip"),
                            paths_.apploadZip, err)) {
        return false;
    }
    if (!verifySha256File(paths_.apploadZip, pinned_.appload.sha256, err)) {
        return false;
    }

    paths_.onDeviceFiles.clear();
    for (const QString& name : onDeviceResourceNames()) {
        const QString src = QStringLiteral(":/on-device/") + name;
        const QString dst = paths_.workDir + QLatin1Char('/') + name;
        if (!copyResourceToFile(src, dst, err)) {
            return false;
        }
        paths_.onDeviceFiles << dst;
    }

    paths_.homeTileDir = paths_.workDir + QStringLiteral("/home-tile");
    if (!QDir().mkpath(paths_.homeTileDir)) {
        if (err) *err = QStringLiteral("cannot create %1").arg(paths_.homeTileDir);
        return false;
    }
    paths_.homeTileFiles.clear();
    for (const QString& name : homeTileResourceNames()) {
        const QString src = QStringLiteral(":/home-tile/") + name;
        const QString dst = paths_.homeTileDir + QLatin1Char('/') + name;
        if (!copyResourceToFile(src, dst, err)) {
            return false;
        }
        paths_.homeTileFiles << dst;
    }

    return true;
}

QByteArray BundledPayload::sha256OfFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f)) return {};
    return hash.result().toHex();
}

QByteArray BundledPayload::sha256OfBytes(const QByteArray& bytes) {
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}

}
