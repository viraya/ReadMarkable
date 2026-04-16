#include "flow/device_state.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace rm::installer::flow {

namespace {

QString getString(const QJsonObject& obj, const char* key) {
    const QJsonValue v = obj.value(QLatin1String(key));
    return v.isString() ? v.toString() : QString();
}

bool getBool(const QJsonObject& obj, const char* key, bool defaultValue = false) {
    const QJsonValue v = obj.value(QLatin1String(key));
    return v.isBool() ? v.toBool() : defaultValue;
}

}

std::optional<DeviceState> DeviceState::fromJson(const QByteArray& json,
                                                 QString* err) {
    QJsonParseError parseErr{};
    QJsonDocument doc = QJsonDocument::fromJson(json, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (err) {
            *err = QStringLiteral("detect-state JSON parse failed: %1")
                       .arg(parseErr.errorString());
        }
        return std::nullopt;
    }
    const QJsonObject root = doc.object();

    DeviceState state;
    state.deviceModel = getString(root, "device_model");
    state.firmware = getString(root, "firmware");
    state.rootfsMount = getString(root, "rootfs_mount");

    const QJsonObject xovi = root.value(QStringLiteral("xovi")).toObject();
    state.xovi.installed = getBool(xovi, "installed");
    state.xovi.tag = getString(xovi, "tag");
    state.xovi.installSource = getString(xovi, "install_source");
    state.xovi.path = getString(xovi, "path");
    state.xovi.triggerInstalled = getBool(xovi, "trigger_installed");

    const QJsonObject appload = root.value(QStringLiteral("appload")).toObject();
    state.appload.installed = getBool(appload, "installed");
    state.appload.tag = getString(appload, "tag");
    state.appload.installSource = getString(appload, "install_source");

    const QJsonObject rm = root.value(QStringLiteral("readmarkable")).toObject();
    state.readmarkable.installed = getBool(rm, "installed");
    state.readmarkable.version = getString(rm, "version");

    if (state.deviceModel.isEmpty()) {
        if (err) *err = QStringLiteral("device_model missing from detect-state JSON");
        return std::nullopt;
    }
    return state;
}

bool DeviceState::isSupportedModel() const {

    return deviceModel.contains(QStringLiteral("Chiappa"), Qt::CaseInsensitive) ||
           deviceModel.contains(QStringLiteral("Ferrari"), Qt::CaseInsensitive);
}

}
