#include "wizard/page_action_summary.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "flow/router.h"
#include "wizard/installer_wizard.h"

namespace rm::installer::wizard {

PageActionSummary::PageActionSummary(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Ready to proceed"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 20, 40, 20);
    layout->setSpacing(18);

    summaryLabel_ = new QLabel(this);
    summaryLabel_->setWordWrap(true);
    summaryLabel_->setTextFormat(Qt::RichText);
    layout->addWidget(summaryLabel_);

    removeXoviCheck_ = new QCheckBox(
        tr("Also remove XOVI (will break any other XOVI apps)"), this);
    removeXoviCheck_->setVisible(false);
    layout->addWidget(removeXoviCheck_);

    bootPersistenceCheck_ = new QCheckBox(
        tr("Enable tap-to-launch after reboot (recommended). After each reboot, "
           "tap the top of the screen three times to bring back the ReadMarkable "
           "tile. Without this, you need to run xovi/start over SSH."),
        this);
    bootPersistenceCheck_->setChecked(true);
    bootPersistenceCheck_->setVisible(false);
    layout->addWidget(bootPersistenceCheck_);

    auto* toggleRow = new QHBoxLayout();
    toggleRow->addStretch();
    toggleUninstallButton_ = new QPushButton(tr("Uninstall instead"), this);
    toggleUninstallButton_->setVisible(false);
    toggleRow->addWidget(toggleUninstallButton_);
    layout->addLayout(toggleRow);
    connect(toggleUninstallButton_, &QPushButton::clicked,
            this, &PageActionSummary::onToggleUninstallClicked);

    layout->addStretch();
}

void PageActionSummary::setVariant(Variant v, const QString& summaryHtml) {
    blocked_ = (v == BlockingError);
    switch (v) {
        case Install:
            setSubTitle(tr("The installer will install ReadMarkable."));
            break;
        case Update:
            setSubTitle(tr("The installer will update ReadMarkable."));
            break;
        case Uninstall:
            setSubTitle(tr("The installer will remove ReadMarkable."));
            break;
        case BlockingError:
            setSubTitle(tr("Cannot proceed, see below."));
            break;
    }
    summaryLabel_->setText(summaryHtml);
    removeXoviCheck_->setVisible(v == Uninstall);
    emit completeChanged();
}

void PageActionSummary::initializePage() {
    auto* w = qobject_cast<InstallerWizard*>(wizard());
    Q_ASSERT(w);
    const auto& d = w->routerDecision();
    switch (d.variant) {
        case flow::FlowVariant::Install:
            setVariant(Install,
                       QStringLiteral("<p>%1</p>").arg(d.summary.toHtmlEscaped()));
            break;
        case flow::FlowVariant::Update:
            setVariant(Update,
                       QStringLiteral("<p>%1</p>").arg(d.summary.toHtmlEscaped()));
            break;
        case flow::FlowVariant::Uninstall:
            setVariant(Uninstall,
                       QStringLiteral("<p>%1</p>").arg(d.summary.toHtmlEscaped()));
            break;
        case flow::FlowVariant::BlockingError:
            setVariant(BlockingError,
                       QStringLiteral("<p><b>%1</b></p><p>%2</p>")
                           .arg(d.summary.toHtmlEscaped(),
                                d.errorDetails.toHtmlEscaped()));
            break;
    }

    setBootPersistenceCheckboxVisible(d.offerBootPersistence &&
                                      (d.variant == flow::FlowVariant::Install ||
                                       d.variant == flow::FlowVariant::Update));

    const bool rmInstalled = w->deviceState().readmarkable.installed;
    if (d.variant == flow::FlowVariant::Update && rmInstalled) {
        toggleUninstallButton_->setText(tr("Uninstall instead"));
        toggleUninstallButton_->setVisible(true);
    } else if (d.variant == flow::FlowVariant::Uninstall) {
        toggleUninstallButton_->setText(tr("Update instead"));
        toggleUninstallButton_->setVisible(true);
    } else {
        toggleUninstallButton_->setVisible(false);
    }
}

void PageActionSummary::onToggleUninstallClicked() {
    auto* w = qobject_cast<InstallerWizard*>(wizard());
    Q_ASSERT(w);
    const auto& d = w->routerDecision();
    if (d.variant == flow::FlowVariant::Uninstall) {
        w->setRouterDecision(flow::decideInstallOrUpdate(w->deviceState(), w->pinned()));
    } else {
        w->setRouterDecision(flow::decideUninstall(w->deviceState()));
    }
    initializePage();
}

bool PageActionSummary::isComplete() const { return !blocked_; }

int PageActionSummary::nextId() const {
    return InstallerWizard::PageInstallingId;
}

void PageActionSummary::setRemoveXoviCheckboxVisible(bool visible) {
    removeXoviCheck_->setVisible(visible);
}

bool PageActionSummary::removeXoviChecked() const {
    return removeXoviCheck_->isVisible() && removeXoviCheck_->isChecked();
}

void PageActionSummary::setBootPersistenceCheckboxVisible(bool visible) {
    bootPersistenceCheck_->setVisible(visible);
}

bool PageActionSummary::bootPersistenceChecked() const {

    return !bootPersistenceCheck_->isVisible() ||
           bootPersistenceCheck_->isChecked();
}

}
