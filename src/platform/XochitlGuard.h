#pragma once

#include <QObject>

class XochitlGuard : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString strategy READ strategyName CONSTANT)
    Q_PROPERTY(bool xochitlStopped READ xochitlStopped NOTIFY stateChanged)

public:
    enum class Strategy {
        SystemctlStopStart,
        SigstopSigcont,
        None,
    };

    explicit XochitlGuard(Strategy strategy = Strategy::SystemctlStopStart,
                         QObject *parent = nullptr);
    ~XochitlGuard();

    QString strategyName() const;
    bool xochitlStopped() const { return m_stopped; }

    Q_INVOKABLE void acquire();
    Q_INVOKABLE void release();

    bool waitForRelease(int timeoutMs = 2000);

    static int s_xochitlPid;

signals:
    void stateChanged();

private:
    void systemctlStop();
    void systemctlStart();
    void sigstopXochitl();
    void sigcontXochitl();
    int findXochitlPid();
    void registerCrashHandlers();

    Strategy m_strategy;
    bool m_stopped = false;
    bool m_acquired = false;
};
