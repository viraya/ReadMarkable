#include "wizard/page_action_summary.h"

#include <QCheckBox>
#include <QLabel>
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
        tr("Add boot persistence (recommended  -  without it, ReadMarkable's "
           "tile disappears after each device reboot)"),
        this);
    bootPersistenceCheck_->setChecked(true);
    bootPersistenceCheck_->setVisible(false);
    layout->addWidget(bootPersistenceCheck_);

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
            setSubTitle(tr("Cannot proceed  -  see below."));
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
