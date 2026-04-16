#include "wizard/page_prereqs.h"

#include <QLabel>
#include <QPushButton>
#include <QTcpSocket>
#include <QTimer>
#include <QVBoxLayout>

namespace rm::installer::wizard {

namespace {
constexpr const char* kBannerText =
    "<i>ReadMarkable installs as a third-party app. Your reMarkable doesn't "
    "need a developer mode or jailbreak  -  only USB web interface enabled. "
    "Your device's normal use, warranty, and reMarkable Cloud sync continue "
    "working.</i>";
}

PagePrereqs::PagePrereqs(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Prerequisites"));
    setSubTitle(tr("Waiting for your device."));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 20, 40, 20);
    layout->setSpacing(14);

    auto* banner = new QLabel(tr(kBannerText), this);
    banner->setWordWrap(true);
    layout->addWidget(banner);

    usbRow_ = new QLabel(this);
    sshRow_ = new QLabel(this);
    layout->addWidget(usbRow_);
    layout->addWidget(sshRow_);

    setRow(usbRow_, false, tr("USB web interface enabled"));
    setRow(sshRow_, false, tr("Device reachable over USB"));

    connectByIpButton_ = new QPushButton(tr("Connect by IP instead"), this);
    connectByIpButton_->setToolTip(
        tr("Skip the USB check  -  for Wi-Fi / LAN users."));
    layout->addWidget(connectByIpButton_, 0, Qt::AlignLeft);
    connect(connectByIpButton_, &QPushButton::clicked, this, [this] {
        usbOk_ = true;
        sshOk_ = true;
        setRow(usbRow_, true, tr("(skipped  -  connecting by IP)"));
        setRow(sshRow_, true, tr("(skipped  -  connecting by IP)"));
        emit completeChanged();
    });

    layout->addStretch();

    poller_ = new QTimer(this);
    poller_->setInterval(1000);
    connect(poller_, &QTimer::timeout, this, &PagePrereqs::pollUsb);
}

void PagePrereqs::initializePage() {
    usbOk_ = false;
    sshOk_ = false;
    setRow(usbRow_, false, tr("USB web interface enabled"));
    setRow(sshRow_, false, tr("Device reachable over USB"));
    poller_->start();
}

bool PagePrereqs::isComplete() const { return usbOk_ && sshOk_; }

void PagePrereqs::pollUsb() {
    QTcpSocket probe;
    probe.connectToHost(QStringLiteral("10.11.99.1"), 22);
    const bool ok = probe.waitForConnected(500);
    probe.abort();
    if (ok != sshOk_) {
        sshOk_ = ok;
        usbOk_ = ok;
        setRow(usbRow_, ok,
               ok ? tr("USB web interface reachable")
                  : tr("USB web interface enabled"));
        setRow(sshRow_, ok,
               ok ? tr("Device reachable at 10.11.99.1:22")
                  : tr("Device reachable over USB"));
        emit completeChanged();
    }
}

void PagePrereqs::setRow(QLabel* row, bool ok, const QString& text) {
    const QString icon = ok ? QStringLiteral("✓") : QStringLiteral("…");
    const QString color = ok ? QStringLiteral("#2a9d4f") : QStringLiteral("#a0a0a0");
    row->setText(QStringLiteral(
                     "<span style='color:%1;font-weight:bold;'>%2</span>  %3")
                     .arg(color, icon, text));
}

}
