#include "wizard/page_done.h"

#include <QDateTime>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "diagnostics/diagnostics_zip.h"
#include "diagnostics/logger.h"
#include "flow/router.h"
#include "wizard/installer_wizard.h"

namespace rm::installer::wizard {

PageDone::PageDone(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Finished"));
    setCommitPage(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 20, 40, 20);
    stack_ = new QStackedWidget(this);
    layout->addWidget(stack_, 1);

    auto* successPage = new QWidget(this);
    auto* successLayout = new QVBoxLayout(successPage);
    successLabel_ = new QLabel(successPage);
    successLabel_->setWordWrap(true);
    successLabel_->setAlignment(Qt::AlignCenter);
    successLabel_->setTextFormat(Qt::RichText);
    successLayout->addStretch();
    successLayout->addWidget(successLabel_);
    successLayout->addStretch();

    auto* failurePage = new QWidget(this);
    auto* failureLayout = new QVBoxLayout(failurePage);
    failureLabel_ = new QLabel(tr("<h2>Install failed</h2>"), failurePage);
    failureLabel_->setAlignment(Qt::AlignCenter);
    failureLayout->addWidget(failureLabel_);
    errorLineLabel_ = new QLabel(failurePage);
    errorLineLabel_->setWordWrap(true);
    errorLineLabel_->setStyleSheet(QStringLiteral("color:#c04040;"));
    failureLayout->addWidget(errorLineLabel_);

    auto* row = new QHBoxLayout();
    retryButton_ = new QPushButton(tr("Retry"), failurePage);
    diagnosticsButton_ = new QPushButton(tr("Save diagnostics…"), failurePage);
    row->addStretch();
    row->addWidget(diagnosticsButton_);
    row->addWidget(retryButton_);
    failureLayout->addLayout(row);
    connect(retryButton_, &QPushButton::clicked, this, &PageDone::retryRequested);
    connect(diagnosticsButton_, &QPushButton::clicked, this, [this] {
        auto* w = qobject_cast<InstallerWizard*>(wizard());
        if (!w) return;
        const QString suggested =
            QStringLiteral("readmarkable-diagnostics-%1.tar.gz")
                .arg(QDateTime::currentDateTimeUtc().toString(
                    QStringLiteral("yyyyMMddTHHmmssZ")));
        const QString path = QFileDialog::getSaveFileName(
            this, tr("Save diagnostics bundle"),
            QDir::homePath() + QLatin1Char('/') + suggested,
            tr("tar.gz archive (*.tar.gz)"));
        if (path.isEmpty()) return;
        QString err;
        const QString logPath =
            w->sessionLog() ? w->sessionLog()->filePath() : QString();
        if (!diagnostics::writeDiagnosticsArchive(
                logPath, w->lastDeviceStateJson(), path, &err)) {
            QMessageBox::warning(this, tr("Diagnostics failed"),
                                 tr("Could not write bundle: %1").arg(err));
        } else {
            QMessageBox::information(
                this, tr("Saved"),
                tr("Diagnostics bundle saved to:\n%1").arg(path));
        }
        emit diagnosticsZipRequested();
    });

    stack_->addWidget(successPage);
    stack_->addWidget(failurePage);
    stack_->setCurrentIndex(0);
}

void PageDone::setSuccess(const QString& headingHtml, const QString& bodyHtml) {
    successLabel_->setText(headingHtml + bodyHtml);
    stack_->setCurrentIndex(0);
}

void PageDone::setFailure(const QString& errorText) {
    errorLineLabel_->setText(errorText.toHtmlEscaped());
    stack_->setCurrentIndex(1);
}

void PageDone::initializePage() {
    auto* w = qobject_cast<InstallerWizard*>(wizard());
    Q_ASSERT(w);
    if (w->flowSucceeded()) {
        switch (w->routerDecision().variant) {
            case flow::FlowVariant::Uninstall:
                setSuccess(
                    QStringLiteral("<h2 style='color:#2a9d4f;'>&#10003; Uninstalled</h2>"),
                    tr("<p style='font-size:13pt;'>ReadMarkable has been removed "
                       "from your device.</p>"
                       "<p><i>Your notebooks, Cloud sync, and reading data are "
                       "untouched. Re-run the installer any time to put "
                       "ReadMarkable back.</i></p>"));
                break;
            case flow::FlowVariant::Update:
                setSuccess(
                    QStringLiteral("<h2 style='color:#2a9d4f;'>&#10003; Updated</h2>"),
                    tr("<p style='font-size:13pt;'>Tap the <b>ReadMarkable</b> "
                       "tile on your device home screen to open the app.</p>"
                       "<p><i>Reading positions and highlights were preserved.</i></p>"));
                break;
            default:
                setSuccess(
                    QStringLiteral("<h2 style='color:#2a9d4f;'>&#10003; Installed</h2>"),
                    tr("<p style='font-size:13pt;'>Tap the <b>ReadMarkable</b> "
                       "tile on your device home screen to open the app.</p>"
                       "<p><i>The tile appears alongside reMarkable's built-in "
                       "apps. Your notebooks and Cloud sync are unaffected.</i></p>"));
                break;
        }
    } else {
        setFailure(w->flowError());
    }
}

}
