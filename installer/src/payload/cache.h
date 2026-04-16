#pragma once

#include <QString>
#include <optional>

namespace rm::installer::payload {

class Cache {
 public:
    Cache();
    explicit Cache(const QString& cacheDir);

    QString tarballPathFor(const QString& version) const;
    QString sha256PathFor(const QString& version) const;

    std::optional<QString> lookup(const QString& version) const;

    bool store(const QString& version, const QString& srcPath,
               QString* err = nullptr);

    void evict(int keep = 3);

    QString dir() const { return cacheDir_; }

 private:
    QString cacheDir_;
    void ensureDir() const;
};

}
