#include "InputDiscovery.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

InputDiscovery::InputDiscovery(QObject *parent) : QObject(parent) {}

void InputDiscovery::discover()
{
    qInfo() << "InputDiscovery: starting device discovery...";
    m_devices = parseInputDevices();
    qInfo() << "InputDiscovery: found" << m_devices.size() << "devices";
    m_penPath.clear();
    m_touchPath.clear();

    for (const auto &dev : m_devices) {
        qInfo() << "Input device:" << dev.name << "at" << dev.eventPath;

        if (dev.name.contains("marker", Qt::CaseInsensitive)
            || dev.name.contains("pen", Qt::CaseInsensitive)
            || dev.name.contains("stylus", Qt::CaseInsensitive)) {
            m_penPath = dev.eventPath;
            qInfo() << "  --> PEN device:" << m_penPath;
        }

        if (dev.name.contains("touch", Qt::CaseInsensitive)
            && !dev.name.contains("marker", Qt::CaseInsensitive)
            && !dev.name.contains("pen", Qt::CaseInsensitive)) {
            m_touchPath = dev.eventPath;
            qInfo() << "  --> TOUCH device:" << m_touchPath;
        }
    }

    if (m_penPath.isEmpty())
        qWarning() << "PEN device not found! Check /proc/bus/input/devices";
    if (m_touchPath.isEmpty())
        qWarning() << "TOUCH device not found! Check /proc/bus/input/devices";

    emit devicesDiscovered();
}

QList<InputDeviceInfo> InputDiscovery::parseInputDevices()
{
    QList<InputDeviceInfo> devices;
    QFile file("/proc/bus/input/devices");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open /proc/bus/input/devices";
        return devices;
    }

    QByteArray content = file.readAll();
    file.close();
    QStringList lines = QString::fromUtf8(content).split('\n');

    qInfo() << "InputDiscovery: read" << content.size() << "bytes," << lines.size() << "lines";

    InputDeviceInfo current;

    for (const QString &line : lines) {

        if (line.startsWith("I:")) {
            if (!current.name.isEmpty()) {
                devices.append(current);
            }
            current = InputDeviceInfo();
        } else if (line.startsWith("N: Name=")) {
            current.name = line.mid(9).remove('"');
        } else if (line.startsWith("H: Handlers=")) {
            current.handlers = line.mid(12);
            current.eventPath = findEventPath(current.handlers);
        } else if (line.startsWith("S: Sysfs=")) {
            current.sysPath = line.mid(9);
        } else if (line.startsWith("B:")) {
            int eq = line.indexOf('=');
            if (eq > 3) {
                QString key = line.mid(3, eq - 3).trimmed();
                QString val = line.mid(eq + 1).trimmed();
                current.properties[key] = val;
            }
        }
    }

    if (!current.name.isEmpty()) {
        devices.append(current);
    }

    qInfo() << "InputDiscovery: parsed" << lines.size() << "lines," << devices.size() << "devices";
    return devices;
}

QString InputDiscovery::findEventPath(const QString &handlers)
{
    static QRegularExpression re("event(\\d+)");
    auto match = re.match(handlers);
    if (match.hasMatch()) {
        return "/dev/input/event" + match.captured(1);
    }
    return {};
}

QVariantList InputDiscovery::allDevices() const
{
    QVariantList list;
    for (const auto &dev : m_devices) {
        QVariantMap map;
        map["name"] = dev.name;
        map["eventPath"] = dev.eventPath;
        map["handlers"] = dev.handlers;
        map["sysPath"] = dev.sysPath;
        map["properties"] = dev.properties;
        list.append(map);
    }
    return list;
}
