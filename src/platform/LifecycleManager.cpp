#include "LifecycleManager.h"
#include "display/DisplayManager.h"

#include <QDBusConnection>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QProcess>
#include <QDebug>

LifecycleManager::LifecycleManager(DisplayManager *display, QObject *parent)
    : QObject(parent), m_display(display)
{
    connectDbusSignals();

    if (auto *app = QGuiApplication::instance()) {
        app->installEventFilter(this);
        qInfo() << "LifecycleManager: power button event filter installed";
    }
}

bool LifecycleManager::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_PowerOff || keyEvent->key() == Qt::Key_Sleep) {
            qInfo() << "LifecycleManager: power button pressed, triggering suspend";
            suspend();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void LifecycleManager::suspend()
{
    qInfo() << "LifecycleManager: requesting system suspend";
    QProcess::startDetached("systemctl", {"suspend"});
}

void LifecycleManager::connectDbusSignals()
{
    bool ok = QDBusConnection::systemBus().connect(
        "org.freedesktop.login1",
        "/org/freedesktop/login1",
        "org.freedesktop.login1.Manager",
        "PrepareForSleep",
        this,
        SLOT(onPrepareForSleep(bool))
    );

    if (ok) {
        qInfo() << "LifecycleManager: connected to PrepareForSleep signal";
    } else {
        qWarning() << "LifecycleManager: FAILED to connect to PrepareForSleep"
                   << "- sleep/wake recovery will not work";
    }
}

void LifecycleManager::onPrepareForSleep(bool goingToSleep)
{
    if (goingToSleep) {
        handleSleep();
    } else {
        handleWake();
    }
}

void LifecycleManager::handleSleep()
{
    qInfo() << "LifecycleManager: device going to sleep";
    m_awake = false;
    emit aboutToSleep();
    emit wakeStateChanged();
}

void LifecycleManager::handleWake()
{
    qInfo() << "LifecycleManager: device waking up";
    m_awake = true;
    m_wakeCount++;

    if (m_display) {
        qInfo() << "LifecycleManager: issuing post-wake Initialize refresh";
        m_display->initialize();
    }

    emit wokeUp();
    emit wakeStateChanged();
}
