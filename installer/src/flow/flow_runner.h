#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <atomic>
#include <functional>

#include "ssh/ssh_client.h"
#include "ssh/ssh_session.h"

namespace rm::installer::flow {

struct Phase {
    QString name;
    bool abortSafe = false;
    std::function<bool(ssh::SshSession& session, QString* err)> run;
};

class FlowRunner : public QObject {
    Q_OBJECT
 public:
    explicit FlowRunner(ssh::SshSession* session, QObject* parent = nullptr);

    void run(const QList<Phase>& phases);
    void abort(const QString& reason = QStringLiteral("user abort"));

 signals:
    void phaseStarted(const QString& name, int index, int total);
    void phaseLogLine(rm::installer::ssh::Stream stream, const QByteArray& line);
    void phaseFinished(const QString& name, bool success, const QString& err);
    void flowFinished(bool success);
    void flowAborted(const QString& reason);

 private:
    ssh::SshSession* session_;
    std::atomic<bool> aborting_{false};
};

}
