#pragma once

#include <QObject>
#include <QEvent>

class DisplayManager;

class LifecycleManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool awake READ awake NOTIFY wakeStateChanged)
    Q_PROPERTY(int wakeCount READ wakeCount NOTIFY wakeStateChanged)

public:
    explicit LifecycleManager(DisplayManager *display, QObject *parent = nullptr);

    bool awake() const { return m_awake; }
    int wakeCount() const { return m_wakeCount; }

    Q_INVOKABLE void suspend();

signals:
    void wakeStateChanged();
    void aboutToSleep();
    void wokeUp();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onPrepareForSleep(bool goingToSleep);

private:
    void connectDbusSignals();
    void handleWake();
    void handleSleep();

    DisplayManager *m_display;
    bool m_awake = true;
    int m_wakeCount = 0;
};
