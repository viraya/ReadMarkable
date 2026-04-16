#include "flow/flow_runner.h"

namespace rm::installer::flow {

FlowRunner::FlowRunner(ssh::SshSession* session, QObject* parent)
    : QObject(parent), session_(session) {
    if (session_) {
        connect(session_, &ssh::SshSession::lineReceived, this,
                &FlowRunner::phaseLogLine);
    }
}

void FlowRunner::run(const QList<Phase>& phases) {
    aborting_ = false;
    const int total = phases.size();
    for (int i = 0; i < total; ++i) {
        if (aborting_) {
            emit flowAborted(QStringLiteral("aborted before phase '%1'")
                                 .arg(phases[i].name));
            return;
        }
        emit phaseStarted(phases[i].name, i, total);

        QString err;
        bool ok = false;
        if (!session_) {
            err = QStringLiteral("no active SSH session");
        } else {
            ok = phases[i].run(*session_, &err);
        }

        emit phaseFinished(phases[i].name, ok, err);
        if (!ok) {
            emit flowFinished(false);
            return;
        }
    }
    emit flowFinished(true);
}

void FlowRunner::abort(const QString& reason) {
    aborting_ = true;
    if (session_) session_->abort();
    emit flowAborted(reason);
}

}
