#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>

struct InputDeviceInfo {
    QString name;
    QString eventPath;
    QString handlers;
    QString sysPath;
    QVariantMap properties;
};

class InputDiscovery : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString penDevicePath READ penDevicePath NOTIFY devicesDiscovered)
    Q_PROPERTY(QString touchDevicePath READ touchDevicePath NOTIFY devicesDiscovered)
    Q_PROPERTY(bool penFound READ penFound NOTIFY devicesDiscovered)
    Q_PROPERTY(bool touchFound READ touchFound NOTIFY devicesDiscovered)
    Q_PROPERTY(QVariantList allDevices READ allDevices NOTIFY devicesDiscovered)

public:
    explicit InputDiscovery(QObject *parent = nullptr);

    Q_INVOKABLE void discover();

    QString penDevicePath() const { return m_penPath; }
    QString touchDevicePath() const { return m_touchPath; }
    bool penFound() const { return !m_penPath.isEmpty(); }
    bool touchFound() const { return !m_touchPath.isEmpty(); }
    QVariantList allDevices() const;

signals:
    void devicesDiscovered();

private:
    QList<InputDeviceInfo> parseInputDevices();
    QString findEventPath(const QString &handlers);

    QString m_penPath;
    QString m_touchPath;
    QList<InputDeviceInfo> m_devices;
};
