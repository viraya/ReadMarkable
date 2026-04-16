#include "wizard/page_installing.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QThread>
#include <QUuid>
#include <QVBoxLayout>

#include "diagnostics/logger.h"
#include "flow/flow_runner.h"
#include "flow/install_flow.h"
#include "flow/router.h"
#include "payload/github_fetcher.h"
#include "ssh/ssh_client.h"
#include "ssh/ssh_session.h"
#include "wizard/installer_wizard.h"
#include "wizard/page_action_summary.h"

namespace rm::installer::wizard {

namespace {

QString makeStagingDir() {
    const QString uid = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    return QStringLiteral("/tmp/rm-install-") + uid;
}

}

PageInstalling::PageInstalling(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Installing"));
    setSubTitle(tr("Working on your device, don't disconnect the cable."));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 20, 40, 20);
    layout->setSpacing(12);

    phaseLabel_ = new QLabel(tr("Preparing…"), this);
    layout->addWidget(phaseLabel_);

    progress_ = new QProgressBar(this);
    progress_->setRange(0, 1);
    progress_->setValue(0);
    layout->addWidget(progress_);

    logPane_ = new QTextEdit(this);
    logPane_->setReadOnly(true);
    logPane_->setLineWrapMode(QTextEdit::NoWrap);
    QFont mono = logPane_->font();
    mono.setFamily(QStringLiteral("Consolas"));
    mono.setStyleHint(QFont::Monospace);
    logPane_->setFont(mono);
    logPane_->setMinimumHeight(200);

    logPane_->setStyleSheet(QStringLiteral(
        "QTextEdit { background-color: #1e1e1e; color: #e0e0e0;"
        "            border: 1px solid #3c3c3c; }"));
    layout->addWidget(logPane_, 1);

    auto* bottom = new QHBoxLayout();
    bottom->addStretch();
    abortButton_ = new QPushButton(tr("Abort"), this);
    connect(abortButton_, &QPushButton::clicked, this, [this] {
        const auto ans = QMessageBox::question(
            this, tr("Abort install?"),
            tr("Will leave the device in a partially-installed state.\n\n"
               "Uninstall required to clean up. Continue?"),
            QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Cancel);
        if (ans == QMessageBox::Yes) {
            if (runner_) runner_->abort();
            emit abortRequested();
        }
    });
    bottom->addWidget(abortButton_);
    layout->addLayout(bottom);
}

PageInstalling::~PageInstalling() {
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait(3000);
    }
}

void PageInstalling::initializePage() {
    auto* w = qobject_cast<InstallerWizard*>(wizard());
    Q_ASSERT(w);

    finished_ = false;
    success_ = false;
    lastErr_.clear();
    logPane_->clear();
    phaseLabel_->setText(tr("Preparing payload…"));
    progress_->setRange(0, 1);
    progress_->setValue(0);
    abortButton_->setEnabled(true);

    QString flowName;
    switch (w->routerDecision().variant) {
        case flow::FlowVariant::Update:
            setTitle(tr("Updating"));
            flowName = QStringLiteral("update");
            break;
        case flow::FlowVariant::Uninstall:
            setTitle(tr("Uninstalling"));
            flowName = QStringLiteral("uninstall");
            break;
        default:
            setTitle(tr("Installing"));
            flowName = QStringLiteral("install");
            break;
    }
    w->openSessionLog(flowName);
    QCoreApplication::processEvents();

    QString err;
    if (!w->ensureBundledExtracted(&err)) {
        markDone(false, tr("Bundled payload extraction failed: %1").arg(err));
        return;
    }

    QString localRmTarball;
    QString releaseTag;

    const QString localOverride =
        qEnvironmentVariable("RM_INSTALLER_LOCAL_TARBALL");
    if (!localOverride.isEmpty()) {
        if (!QFile::exists(localOverride)) {
            markDone(false, tr("RM_INSTALLER_LOCAL_TARBALL set but file not "
                               "found: %1").arg(localOverride));
            return;
        }
        static const QRegularExpression re(
            QStringLiteral("^readmarkable-(.+)-chiappa\\.tar\\.gz$"));
        const auto m = re.match(QFileInfo(localOverride).fileName());
        if (!m.hasMatch()) {
            markDone(false,
                     tr("RM_INSTALLER_LOCAL_TARBALL filename must match "
                        "readmarkable-<tag>-chiappa.tar.gz, got '%1'")
                         .arg(QFileInfo(localOverride).fileName()));
            return;
        }
        releaseTag = m.captured(1);
        localRmTarball = localOverride;
        appendLogLine(tr("DEV MODE: using local tarball %1 (tag=%2)")
                          .arg(localOverride, releaseTag));
    } else {
        payload::GithubFetcher fetcher;
        payload::ReleaseInfo rel;
        appendLogLine(tr("Querying GitHub for latest ReadMarkable release…"));
        QCoreApplication::processEvents();
        if (!fetcher.fetchLatestRelease(QStringLiteral("viraya/ReadMarkable"),
                                         rel, {}, &err)) {
            markDone(false, tr("GitHub API failed: %1").arg(err));
            return;
        }
        appendLogLine(tr("Latest release: %1").arg(rel.tag));
        releaseTag = rel.tag;

        payload::ReleaseAsset tarballAsset;
        payload::ReleaseAsset shaAsset;
        for (const auto& a : rel.assets) {
            if (a.name.endsWith(QStringLiteral("-chiappa.tar.gz")))
                tarballAsset = a;
            if (a.name.endsWith(QStringLiteral("-chiappa.tar.gz.sha256")))
                shaAsset = a;
        }
        if (tarballAsset.downloadUrl.isEmpty() ||
            shaAsset.downloadUrl.isEmpty()) {
            markDone(false,
                     tr("GitHub release missing chiappa tarball or .sha256"));
            return;
        }
        if (auto cached = w->cache()->lookup(rel.tag)) {
            appendLogLine(tr("Cached tarball found at %1").arg(*cached));
            localRmTarball = *cached;
        } else {
            appendLogLine(tr("Downloading %1 (%2 bytes)…")
                              .arg(tarballAsset.name,
                                   QString::number(tarballAsset.size)));
            const QString tmpDst =
                QDir::tempPath() + QLatin1Char('/') + tarballAsset.name;
            auto onProgress = [this](qint64 got, qint64 total) {
                if (total > 0) {
                    progress_->setRange(0, static_cast<int>(total));
                    progress_->setValue(static_cast<int>(got));
                }
                QCoreApplication::processEvents();
            };
            if (!fetcher.downloadAssetWithSha256(
                    tarballAsset.downloadUrl, shaAsset.downloadUrl, tmpDst,
                    onProgress, &err)) {
                markDone(false, tr("Tarball download failed: %1").arg(err));
                return;
            }
            if (!w->cache()->store(rel.tag, tmpDst, &err)) {
                markDone(false, tr("Cache store failed: %1").arg(err));
                return;
            }
            localRmTarball = *w->cache()->lookup(rel.tag);
        }
    }

    const auto& dec = w->routerDecision();
    QList<flow::Phase> phases;
    const QString stagingDir = makeStagingDir();

    auto* summaryPage = qobject_cast<PageActionSummary*>(
        wizard()->page(InstallerWizard::PageActionSummaryId));
    const bool userWantsPersistence =
        summaryPage ? summaryPage->bootPersistenceChecked() : true;

    QStringList persistenceFiles;
    for (const QString& p : w->bundledPayload()->paths().onDeviceFiles) {
        if (p.endsWith(QStringLiteral("install-trigger.sh")) ||
            p.endsWith(QStringLiteral("xovi-trigger.service")) ||
            p.endsWith(QStringLiteral("xovi-trigger"))) {
            persistenceFiles << p;
        }
    }

    if (dec.variant == flow::FlowVariant::Install) {
        flow::InstallFlowInputs in;
        in.remoteStagingDir = stagingDir;
        in.localXoviTarball = w->bundledPayload()->paths().xoviTarball;
        in.localApploadZip = w->bundledPayload()->paths().apploadZip;
        in.localRmTarball = localRmTarball;
        in.localOnDeviceFiles = w->bundledPayload()->paths().onDeviceFiles;
        in.localHomeTileFiles = w->bundledPayload()->paths().homeTileFiles;
        in.xoviTag = w->pinned().xovi.tag;
        in.apploadTag = w->pinned().appload.tag;
        in.skipXoviInstall = dec.skipXoviInstall;
        in.skipAppLoadInstall = dec.skipAppLoadInstall;
        in.installPersistence = userWantsPersistence;
        phases = flow::buildInstallFlow(in);
    } else if (dec.variant == flow::FlowVariant::Update) {
        flow::UpdateFlowInputs in;
        in.remoteStagingDir = stagingDir;
        in.localRmTarball = localRmTarball;

        for (const QString& p : w->bundledPayload()->paths().onDeviceFiles) {
            if (p.endsWith(QStringLiteral("install-readmarkable.sh"))) {
                in.installRmScript = p;
            } else if (p.endsWith(QStringLiteral("rebuild-hashtab.sh"))) {
                in.rebuildHashtabScript = p;
            } else if (p.endsWith(QStringLiteral("install-home-tile.sh"))) {
                in.installHomeTileScript = p;
            }
        }
        in.homeTileFiles = w->bundledPayload()->paths().homeTileFiles;

        in.installPersistence = dec.offerBootPersistence && userWantsPersistence;
        in.persistenceFiles = persistenceFiles;
        phases = flow::buildUpdateFlow(in);
    } else if (dec.variant == flow::FlowVariant::Uninstall) {
        flow::UninstallFlowInputs in;
        in.remoteStagingDir = stagingDir;
        in.localOnDeviceFiles = w->bundledPayload()->paths().onDeviceFiles;
        in.alsoRemoveXovi = summaryPage ? summaryPage->removeXoviChecked() : false;
        phases = flow::buildUninstallFlow(in);
    } else {
        markDone(false, tr("Cannot start flow, router returned variant %1")
                             .arg(static_cast<int>(dec.variant)));
        return;
    }

    total_ = phases.size();
    progress_->setRange(0, total_);
    progress_->setValue(0);

    if (!w->session()) {
        markDone(false, tr("No active SSH session."));
        return;
    }

    workerThread_ = new QThread(this);
    auto* session = w->session();
    session->moveToThread(workerThread_);

    runner_ = new flow::FlowRunner(session);
    runner_->moveToThread(workerThread_);

    connect(runner_, &flow::FlowRunner::phaseStarted, this,
            &PageInstalling::onPhaseStarted);
    connect(runner_, &flow::FlowRunner::phaseLogLine, this,
            &PageInstalling::onPhaseLogLine);
    connect(runner_, &flow::FlowRunner::phaseFinished, this,
            &PageInstalling::onPhaseFinished);
    connect(runner_, &flow::FlowRunner::flowFinished, this,
            &PageInstalling::onFlowFinished);

    connect(workerThread_, &QThread::started, runner_,
            [this, phases]() { runner_->run(phases); });
    connect(workerThread_, &QThread::finished, runner_, &QObject::deleteLater);

    workerThread_->start();
}

bool PageInstalling::isComplete() const { return finished_; }

int PageInstalling::nextId() const {
    return InstallerWizard::PageDoneId;
}

void PageInstalling::resetForRun(int totalPhases) {
    finished_ = false;
    total_ = totalPhases > 0 ? totalPhases : 1;
    progress_->setRange(0, total_);
    progress_->setValue(0);
    phaseLabel_->setText(tr("Preparing…"));
    logPane_->clear();
    abortButton_->setEnabled(true);
}

void PageInstalling::setPhase(int index, const QString& name) {
    progress_->setValue(index);
    phaseLabel_->setText(tr("Phase %1 of %2: %3")
                             .arg(index + 1).arg(total_).arg(name));
}

void PageInstalling::appendLogLine(const QString& text, bool isError) {
    const QString escaped = text.toHtmlEscaped();

    const QString color = isError ? QStringLiteral("#ff7070")
                                  : QStringLiteral("#e0e0e0");
    logPane_->append(
        QStringLiteral("<span style='color:%1'>%2</span>").arg(color, escaped));
}

void PageInstalling::markDone(bool success, const QString& finalErr) {
    finished_ = true;
    success_ = success;
    lastErr_ = finalErr;
    abortButton_->setEnabled(false);
    progress_->setValue(total_);
    phaseLabel_->setText(success ? tr("Done, all phases completed.")
                                 : tr("Failed, see log."));
    if (!finalErr.isEmpty()) appendLogLine(finalErr, true);
    emit completeChanged();
}

void PageInstalling::onPhaseStarted(const QString& name, int index, int total) {
    total_ = total;
    progress_->setRange(0, total);
    setPhase(index, name);
    appendLogLine(QStringLiteral("=== %1 ===").arg(name));
    if (auto* w = qobject_cast<InstallerWizard*>(wizard()); w && w->sessionLog()) {
        w->sessionLog()->writePhaseMarker(name);
    }
}

void PageInstalling::onPhaseLogLine(rm::installer::ssh::Stream stream,
                                    const QByteArray& line) {
    const bool isError = (stream == rm::installer::ssh::Stream::StdErr);
    const QString text = QString::fromUtf8(line);
    appendLogLine(text, isError);
    if (auto* w = qobject_cast<InstallerWizard*>(wizard()); w && w->sessionLog()) {
        if (isError) w->sessionLog()->writeError(text);
        else w->sessionLog()->write(text);
    }
}

void PageInstalling::onPhaseFinished(const QString& name, bool success,
                                     const QString& err) {
    if (!success) {
        appendLogLine(QStringLiteral("%1 FAILED: %2").arg(name, err), true);
        lastErr_ = err;
        if (auto* w = qobject_cast<InstallerWizard*>(wizard()); w && w->sessionLog()) {
            w->sessionLog()->writeError(
                QStringLiteral("%1 FAILED: %2").arg(name, err));
        }
    }
}

void PageInstalling::onFlowFinished(bool success) {
    markDone(success, lastErr_);
    auto* w = qobject_cast<InstallerWizard*>(wizard());
    if (w) w->setFlowResult(success, lastErr_);
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait(3000);
        workerThread_->deleteLater();
        workerThread_ = nullptr;
    }
    runner_ = nullptr;
    if (success && wizard()) {
        wizard()->next();
    }
}

}
