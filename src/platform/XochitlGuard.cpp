#include "XochitlGuard.h"

#include <QProcess>
#include <QDebug>
#include <QElapsedTimer>
#include <QThread>
#include <csignal>
#include <sys/types.h>
#include <signal.h>

int XochitlGuard::s_xochitlPid = 0;

static XochitlGuard *s_instance = nullptr;

static void crashHandler(int sig)
{

    if (XochitlGuard::s_xochitlPid > 0) {
        ::kill(XochitlGuard::s_xochitlPid, SIGCONT);
    }

    ::signal(sig, SIG_DFL);
    ::raise(sig);
}

XochitlGuard::XochitlGuard(Strategy strategy, QObject *parent)
    : QObject(parent), m_strategy(strategy)
{
    s_instance = this;
    registerCrashHandlers();
    acquire();
}

XochitlGuard::~XochitlGuard()
{
    release();
    s_instance = nullptr;
}

void XochitlGuard::registerCrashHandlers()
{
    ::signal(SIGSEGV, crashHandler);
    ::signal(SIGABRT, crashHandler);
    ::signal(SIGTERM, crashHandler);
    qInfo() << "XochitlGuard: crash handlers registered (SIGSEGV, SIGABRT, SIGTERM)";
}

QString XochitlGuard::strategyName() const
{
    switch (m_strategy) {
    case Strategy::SystemctlStopStart: return "systemctl stop/start";
    case Strategy::SigstopSigcont: return "SIGSTOP/SIGCONT";
    case Strategy::None: return "none";
    }
    return "unknown";
}

void XochitlGuard::acquire()
{
    if (m_acquired) return;

    switch (m_strategy) {
    case Strategy::SystemctlStopStart:
        systemctlStop();
        break;
    case Strategy::SigstopSigcont:
        sigstopXochitl();
        break;
    case Strategy::None:
        qInfo() << "XochitlGuard: strategy=none, not stopping Xochitl";
        break;
    }

    m_acquired = true;
    m_stopped = true;
    emit stateChanged();
}

void XochitlGuard::release()
{
    if (!m_acquired) return;

    switch (m_strategy) {
    case Strategy::SystemctlStopStart:
        systemctlStart();
        break;
    case Strategy::SigstopSigcont:
        sigcontXochitl();
        break;
    case Strategy::None:
        break;
    }

    m_acquired = false;
    m_stopped = false;
    emit stateChanged();
}

void XochitlGuard::systemctlStop()
{
    qInfo() << "XochitlGuard: stopping Xochitl via systemctl...";
    int result = QProcess::execute("systemctl", {"stop", "xochitl"});
    if (result == 0) {
        qInfo() << "XochitlGuard: Xochitl stopped";
    } else {
        qWarning() << "XochitlGuard: systemctl stop xochitl returned" << result
                   << "(may already be stopped)";
    }
}

void XochitlGuard::systemctlStart()
{
    qInfo() << "XochitlGuard: restarting Xochitl via systemctl...";
    int result = QProcess::execute("systemctl", {"start", "xochitl"});
    if (result == 0) {
        qInfo() << "XochitlGuard: Xochitl restarted";
    } else {
        qWarning() << "XochitlGuard: systemctl start xochitl returned" << result;
    }
}

void XochitlGuard::sigstopXochitl()
{
    int pid = findXochitlPid();
    if (pid > 0) {

        s_xochitlPid = pid;
        qInfo() << "XochitlGuard: sending SIGSTOP to Xochitl pid" << pid;
        ::kill(pid, SIGSTOP);
    } else {
        qWarning() << "XochitlGuard: Xochitl PID not found";
    }
}

void XochitlGuard::sigcontXochitl()
{

    int pid = (s_xochitlPid > 0) ? s_xochitlPid : findXochitlPid();
    if (pid > 0) {
        qInfo() << "XochitlGuard: sending SIGCONT to Xochitl pid" << pid;
        ::kill(pid, SIGCONT);

        s_xochitlPid = 0;
    } else {
        qWarning() << "XochitlGuard: Xochitl PID not found for SIGCONT";
    }
}

bool XochitlGuard::waitForRelease(int timeoutMs)
{
    const int intervalMs = 100;
    const int drmBufferMs = 500;

    QElapsedTimer elapsed;
    elapsed.start();

    int iterations = 0;
    while (elapsed.elapsed() < timeoutMs) {
        int pid = findXochitlPid();
        if (pid <= 0) {
            qInfo() << "XochitlGuard::waitForRelease: xochitl exited after"
                    << elapsed.elapsed() << "ms (" << iterations
                    << "iterations); sleeping" << drmBufferMs << "ms DRM buffer";
            QThread::msleep(drmBufferMs);
            return true;
        }
        qInfo() << "XochitlGuard::waitForRelease: xochitl still alive pid=" << pid
                << "elapsed=" << elapsed.elapsed() << "ms";
        QThread::msleep(intervalMs);
        ++iterations;
    }

    int pid = findXochitlPid();
    qWarning() << "XochitlGuard::waitForRelease: TIMEOUT after" << timeoutMs
               << "ms (iterations=" << iterations << "pid=" << pid
               << "), continuing anyway, framebuffer lock may fail";
    return false;
}

int XochitlGuard::findXochitlPid()
{
    QProcess proc;
    proc.start("pidof", {"xochitl"});
    proc.waitForFinished(3000);
    QString output = proc.readAllStandardOutput().trimmed();
    bool ok = false;
    int pid = output.toInt(&ok);
    return ok ? pid : -1;
}
