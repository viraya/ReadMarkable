#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <memory>

#include "core/pinned_versions.h"

class QTemporaryDir;

namespace rm::installer::payload {

struct ExtractedPaths {
    QString workDir;
    QString xoviTarball;
    QString apploadZip;

    QStringList onDeviceFiles;

    QStringList homeTileFiles;
    QString homeTileDir;
};

class BundledPayload {
 public:
    explicit BundledPayload(const core::PinnedConfig& pinned);
    ~BundledPayload();
    BundledPayload(const BundledPayload&) = delete;
    BundledPayload& operator=(const BundledPayload&) = delete;

    bool extractAll(QString* err = nullptr);

    const ExtractedPaths& paths() const noexcept { return paths_; }

    static QByteArray sha256OfFile(const QString& path);
    static QByteArray sha256OfBytes(const QByteArray& bytes);

 private:
    core::PinnedConfig pinned_;
    std::unique_ptr<QTemporaryDir> work_;
    ExtractedPaths paths_;
};

}
