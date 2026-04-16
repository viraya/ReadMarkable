#pragma once

#include <QString>
#include <QWizardPage>

class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;
class QThread;

namespace rm::installer::flow {
class FlowRunner;
}
namespace rm::installer::ssh {
enum class Stream;
}

namespace rm::installer::wizard {

class PageInstalling : public QWizardPage {
    Q_OBJECT
 public:
    explicit PageInstalling(QWidget* parent = nullptr);
    ~PageInstalling() override;

    void initializePage() override;
    bool isComplete() const override;
    int nextId() const override;

    void resetForRun(int totalPhases);
    void setPhase(int index, const QString& name);
    void appendLogLine(const QString& text, bool isError = false);
    void markDone(bool success, const QString& finalErr = {});

 signals:
    void abortRequested();

 private slots:
    void onPhaseStarted(const QString& name, int index, int total);
    void onPhaseLogLine(rm::installer::ssh::Stream stream,
                        const QByteArray& line);
    void onPhaseFinished(const QString& name, bool success, const QString& err);
    void onFlowFinished(bool success);

 private:
    QProgressBar* progress_ = nullptr;
    QLabel* phaseLabel_ = nullptr;
    QTextEdit* logPane_ = nullptr;
    QPushButton* abortButton_ = nullptr;
    bool finished_ = false;
    bool success_ = false;
    QString lastErr_;
    int total_ = 1;

    QThread* workerThread_ = nullptr;
    rm::installer::flow::FlowRunner* runner_ = nullptr;
};

}
