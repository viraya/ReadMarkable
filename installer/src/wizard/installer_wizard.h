#pragma once

#include <QWizard>
#include <memory>

#include "core/pinned_versions.h"
#include "credentials/device_store.h"
#include "diagnostics/logger.h"
#include "flow/device_state.h"
#include "flow/router.h"
#include "payload/bundled_payload.h"
#include "payload/cache.h"
#include "ssh/known_hosts.h"
#include "ssh/ssh_session.h"

namespace rm::installer::wizard {

class InstallerWizard : public QWizard {
    Q_OBJECT
 public:
    enum PageId {
        PageWelcomeId = 0,
        PagePrereqsId,
        PageConnectId,
        PageActionSummaryId,
        PageInstallingId,
        PageDoneId,
    };

    explicit InstallerWizard(QWidget* parent = nullptr);
    ~InstallerWizard() override;

    const core::PinnedConfig& pinned();
    ssh::KnownHosts* knownHosts() { return knownHosts_.get(); }
    credentials::DeviceStore* deviceStore() { return deviceStore_.get(); }
    payload::Cache* cache() { return cache_.get(); }
    payload::BundledPayload* bundledPayload() { return bundledPayload_.get(); }

    void setSession(std::unique_ptr<ssh::SshSession> session);
    ssh::SshSession* session() { return session_.get(); }

    void setDeviceState(const flow::DeviceState& state) { deviceState_ = state; }
    const flow::DeviceState& deviceState() const { return deviceState_; }

    void setRouterDecision(const flow::RouterDecision& d) { routerDecision_ = d; }
    const flow::RouterDecision& routerDecision() const { return routerDecision_; }

    void setFlowResult(bool success, const QString& err) {
        flowSuccess_ = success;
        flowErr_ = err;
    }
    bool flowSucceeded() const { return flowSuccess_; }
    QString flowError() const { return flowErr_; }

    bool ensureBundledExtracted(QString* err);

 private:
    std::optional<core::PinnedConfig> pinned_;
    std::unique_ptr<ssh::KnownHosts> knownHosts_;
    std::unique_ptr<credentials::DeviceStore> deviceStore_;
    std::unique_ptr<payload::Cache> cache_;
    std::unique_ptr<payload::BundledPayload> bundledPayload_;
    std::unique_ptr<ssh::SshSession> session_;
    flow::DeviceState deviceState_;
    flow::RouterDecision routerDecision_;
    bool payloadExtracted_ = false;
    bool flowSuccess_ = false;
    QString flowErr_;

 public:

    void openSessionLog(const QString& flowName);
    diagnostics::Logger* sessionLog() { return logger_.get(); }

    void setLastDeviceStateJson(const QByteArray& json) {
        lastDeviceStateJson_ = QString::fromUtf8(json);
    }
    QString lastDeviceStateJson() const { return lastDeviceStateJson_; }

 private:
    std::unique_ptr<diagnostics::Logger> logger_;
    QString lastDeviceStateJson_;
};

}
