#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

namespace rm::installer::flow {

struct XoviState {
    bool installed = false;
    QString tag;
    QString installSource;
    QString path;
    bool bootPersistence = false;
};

struct AppLoadState {
    bool installed = false;
    QString tag;
    QString installSource;
};

struct ReadMarkableState {
    bool installed = false;
    QString version;
};

struct DeviceState {
    QString deviceModel;
    QString firmware;
    QString rootfsMount;
    XoviState xovi;
    AppLoadState appload;
    ReadMarkableState readmarkable;

    static std::optional<DeviceState> fromJson(const QByteArray& json,
                                                QString* err = nullptr);

    bool isSupportedModel() const;
};

}
