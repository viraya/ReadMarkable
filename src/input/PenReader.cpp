#include "PenReader.h"

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <QDebug>

PenReader::PenReader(QObject *parent) : QObject(parent) {}

PenReader::~PenReader() { stop(); }

bool PenReader::start(const QString &devicePath)
{
    if (m_running) return true;

    int fd = open(devicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        qWarning() << "PenReader: cannot open" << devicePath;
        return false;
    }

    queryAbsInfo(fd);
    close(fd);

    m_running = true;
    m_active = true;
    emit activeChanged();

    QThread *thread = QThread::create([this, devicePath]() {
        readLoop(devicePath);
    });
    thread->start();

    qInfo() << "PenReader started on" << devicePath
            << "maxX:" << m_maxX << "maxY:" << m_maxY
            << "maxPressure:" << m_maxPressure;
    return true;
}

void PenReader::stop()
{
    m_running = false;
    m_active = false;
    emit activeChanged();
}

void PenReader::queryAbsInfo(int fd)
{
    struct input_absinfo absinfo;

    if (ioctl(fd, EVIOCGABS(ABS_X), &absinfo) == 0) {
        m_minX = absinfo.minimum;
        m_maxX = absinfo.maximum;
        QString info = QString("ABS_X: min=%1 max=%2 res=%3")
            .arg(absinfo.minimum).arg(absinfo.maximum).arg(absinfo.resolution);
        qInfo() << "PenReader:" << info;
        emit evdevInfo(info);
    }

    if (ioctl(fd, EVIOCGABS(ABS_Y), &absinfo) == 0) {
        m_minY = absinfo.minimum;
        m_maxY = absinfo.maximum;
        QString info = QString("ABS_Y: min=%1 max=%2 res=%3")
            .arg(absinfo.minimum).arg(absinfo.maximum).arg(absinfo.resolution);
        qInfo() << "PenReader:" << info;
        emit evdevInfo(info);
    }

    if (ioctl(fd, EVIOCGABS(ABS_PRESSURE), &absinfo) == 0) {
        m_maxPressure = absinfo.maximum;
        QString info = QString("ABS_PRESSURE: min=%1 max=%2")
            .arg(absinfo.minimum).arg(absinfo.maximum);
        qInfo() << "PenReader:" << info;
        emit evdevInfo(info);
    }

    if (ioctl(fd, EVIOCGABS(ABS_TILT_X), &absinfo) == 0) {
        QString info = QString("ABS_TILT_X: min=%1 max=%2")
            .arg(absinfo.minimum).arg(absinfo.maximum);
        qInfo() << "PenReader:" << info;
        emit evdevInfo(info);
    }

    if (ioctl(fd, EVIOCGABS(ABS_TILT_Y), &absinfo) == 0) {
        QString info = QString("ABS_TILT_Y: min=%1 max=%2")
            .arg(absinfo.minimum).arg(absinfo.maximum);
        qInfo() << "PenReader:" << info;
        emit evdevInfo(info);
    }
}

void PenReader::readLoop(const QString &devicePath)
{
    int fd = open(devicePath.toLocal8Bit().constData(), O_RDONLY);
    if (fd < 0) {
        qWarning() << "PenReader: cannot open for reading:" << devicePath;
        m_running = false;
        m_active = false;
        emit activeChanged();
        return;
    }

    struct input_event ev;
    while (m_running) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n != sizeof(ev)) continue;

        if (ev.type == EV_ABS) {
            switch (ev.code) {
            case ABS_X:
                m_x = ev.value;
                break;
            case ABS_Y:
                m_y = ev.value;
                break;
            case ABS_PRESSURE:
                m_pressure = ev.value;
                m_penDown = (ev.value > 0);
                break;
            case ABS_TILT_X:
                m_tiltX = ev.value;
                break;
            case ABS_TILT_Y:
                m_tiltY = ev.value;
                break;
            }
        } else if (ev.type == EV_KEY) {
            if (ev.code == BTN_TOOL_PEN) {
                m_inRange = (ev.value == 1);
                m_isEraser = false;
            } else if (ev.code == BTN_TOOL_RUBBER) {
                m_inRange = (ev.value == 1);
                m_isEraser = (ev.value == 1);
            }
        } else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
            emit penEvent();
        }
    }

    close(fd);
}
