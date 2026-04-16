#include "wizard/installer_wizard.h"

#include <QString>

#include "wizard/page_action_summary.h"
#include "wizard/page_connect.h"
#include "wizard/page_done.h"
#include "wizard/page_installing.h"
#include "wizard/page_prereqs.h"
#include "wizard/page_welcome.h"

namespace rm::installer::wizard {

InstallerWizard::InstallerWizard(QWidget* parent) : QWizard(parent) {
    setWindowTitle(tr("ReadMarkable Installer"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::NoBackButtonOnStartPage, true);
    setOption(QWizard::NoCancelButtonOnLastPage, true);
    resize(720, 520);

    knownHosts_ = std::make_unique<ssh::KnownHosts>();
    deviceStore_ = std::make_unique<credentials::DeviceStore>();
    cache_ = std::make_unique<payload::Cache>();

    setPage(PageWelcomeId, new PageWelcome(this));
    setPage(PagePrereqsId, new PagePrereqs(this));
    setPage(PageConnectId, new PageConnect(this));
    setPage(PageActionSummaryId, new PageActionSummary(this));
    setPage(PageInstallingId, new PageInstalling(this));
    setPage(PageDoneId, new PageDone(this));

    setStartId(PageWelcomeId);
}

InstallerWizard::~InstallerWizard() = default;

const core::PinnedConfig& InstallerWizard::pinned() {
    if (!pinned_) {
        QString err;
        auto cfg = core::PinnedConfig::loadFromResource(
            QStringLiteral(":/config/pinned-versions.toml"), &err);
        if (!cfg) {
            qFatal("pinned-versions.toml missing from Qt resources: %s",
                   qPrintable(err));
        }
        pinned_ = std::move(*cfg);
    }
    return *pinned_;
}

void InstallerWizard::setSession(std::unique_ptr<ssh::SshSession> session) {
    session_ = std::move(session);
}

bool InstallerWizard::ensureBundledExtracted(QString* err) {
    if (payloadExtracted_) return true;
    bundledPayload_ = std::make_unique<payload::BundledPayload>(pinned());
    if (!bundledPayload_->extractAll(err)) {
        bundledPayload_.reset();
        return false;
    }
    payloadExtracted_ = true;
    return true;
}

void InstallerWizard::openSessionLog(const QString& flowName) {
    diagnostics::Logger::evict(20);
    logger_ = diagnostics::Logger::openForFlow(flowName);
}

}
