#include "wizard/page_connect.h"

#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "credentials/auth_handler.h"
#include "flow/device_state.h"
#include "flow/router.h"
#include "wizard/installer_wizard.h"

namespace rm::installer::wizard {

PageConnect::PageConnect(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Connect to your reMarkable"));
    setSubTitle(tr("Enter the SSH credentials from your device."));

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(40, 20, 40, 20);

    auto* form = new QFormLayout();
    outer->addLayout(form);

    ipEdit_ = new QLineEdit(QStringLiteral("10.11.99.1"), this);
    form->addRow(tr("IP address"), ipEdit_);
    connect(ipEdit_, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);

    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText(
        tr("From Settings → Help → About → Copyrights & licenses "
           "(leave blank to use saved device key)"));
    form->addRow(tr("Password"), passwordEdit_);
    connect(passwordEdit_, &QLineEdit::textChanged, this,
            &QWizardPage::completeChanged);

    rememberCheck_ = new QCheckBox(tr("Remember this device"), this);
    rememberCheck_->setToolTip(
        tr("Saves a per-device SSH key so future installs don't need the "
           "password. Fingerprint-keyed, safe across USB/LAN."));
    rememberCheck_->setChecked(true);
    outer->addWidget(rememberCheck_);

    findPasswordButton_ = new QPushButton(tr("Find my password…"), this);
    findPasswordButton_->setFlat(true);
    connect(findPasswordButton_, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("http://10.11.99.1/")));
    });
    outer->addWidget(findPasswordButton_, 0, Qt::AlignLeft);

    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);
    outer->addWidget(statusLabel_);

    outer->addStretch();

    registerField(QStringLiteral("connect.ip*"), ipEdit_);
    registerField(QStringLiteral("connect.password"), passwordEdit_);
    registerField(QStringLiteral("connect.remember"), rememberCheck_);
}

bool PageConnect::isComplete() const {

    return !ipEdit_->text().isEmpty();
}

int PageConnect::nextId() const {
    return InstallerWizard::PageActionSummaryId;
}

void PageConnect::showStatus(const QString& message, bool isError) {
    const QString color = isError ? QStringLiteral("#c04040")
                                  : QStringLiteral("#3060a0");
    statusLabel_->setText(
        QStringLiteral("<span style='color:%1'>%2</span>").arg(color,
                                                               message.toHtmlEscaped()));
    QApplication::processEvents();
}

bool PageConnect::validatePage() {
    auto* w = qobject_cast<InstallerWizard*>(wizard());
    Q_ASSERT(w);

    setEnabled(false);
    showStatus(tr("Connecting…"), false);

    credentials::AuthHandler auth(w->deviceStore(), w->knownHosts());
    credentials::AuthHandler::Request req;
    req.host = ipEdit_->text();
    req.user = QStringLiteral("root");
    req.password = passwordEdit_->text();
    req.rememberDevice = rememberCheck_->isChecked();
    auto result = auth.tryConnect(req);

    if (result.status != credentials::AuthHandler::Status::Ok) {
        showStatus(tr("Connection failed: %1").arg(result.err), true);
        setEnabled(true);
        return false;
    }

    showStatus(tr("Connected. Probing device state…"), false);

    QString err;
    if (!w->ensureBundledExtracted(&err)) {
        showStatus(tr("Bundled payload extraction failed: %1").arg(err), true);
        setEnabled(true);
        return false;
    }

    const QStringList& onDev = w->bundledPayload()->paths().onDeviceFiles;
    QString detectStatePath;
    for (const QString& p : onDev) {
        if (p.endsWith(QStringLiteral("detect-state.sh"))) {
            detectStatePath = p;
            break;
        }
    }
    if (detectStatePath.isEmpty() ||
        !result.session->sftpPut(detectStatePath,
                                 QStringLiteral("/tmp/rm-detect-state.sh"), 0755,
                                 &err)) {
        showStatus(tr("Uploading detect-state.sh failed: %1").arg(err), true);
        setEnabled(true);
        return false;
    }
    const auto exec = result.session->exec(
        QStringLiteral("sh /tmp/rm-detect-state.sh"));
    if (exec.exitCode != 0) {
        showStatus(tr("detect-state.sh failed: %1")
                       .arg(QString::fromUtf8(exec.stdErr)),
                   true);
        setEnabled(true);
        return false;
    }
    auto state = flow::DeviceState::fromJson(exec.stdOut, &err);
    if (!state) {
        showStatus(tr("detect-state JSON parse failed: %1").arg(err), true);
        setEnabled(true);
        return false;
    }
    w->setDeviceState(*state);
    w->setLastDeviceStateJson(exec.stdOut);
    w->setRouterDecision(flow::decideInstallOrUpdate(*state, w->pinned()));
    w->setSession(std::move(result.session));
    setEnabled(true);
    return true;
}

}
