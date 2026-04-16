#pragma once

#include <QByteArray>
#include <QString>
#include <optional>
#include <string>

namespace rm::installer::core {

struct PinnedXovi {
    QString tag;
    QString url;
    QString sha256;
    QString minimumCompatibleTag;
};

struct PinnedAppLoad {
    QString tag;
    QString url;
    QString sha256;
};

struct PinnedReadMarkable {
    QString urlTemplate;
    QString sha256UrlTemplate;
    QString minVersion;
};

struct PinnedConfig {
    PinnedXovi xovi;
    PinnedAppLoad appload;
    PinnedReadMarkable readmarkable;

    static std::optional<PinnedConfig> loadFromString(const QByteArray& toml,
                                                      QString* err = nullptr);
    static std::optional<PinnedConfig> loadFromResource(const QString& resourcePath,
                                                        QString* err = nullptr);
    static std::optional<PinnedConfig> loadFromFile(const QString& path,
                                                    QString* err = nullptr);

    static int compareXoviTag(const QString& a, const QString& b);

    QString resolveReadMarkableUrl(const QString& tag) const;
    QString resolveReadMarkableSha256Url(const QString& tag) const;
};

}
