#pragma once

#include <QString>
#include <QWizardPage>

class QLabel;
class QPushButton;
class QTimer;

namespace rm::installer::wizard {

class PagePrereqs : public QWizardPage {
    Q_OBJECT
 public:
    explicit PagePrereqs(QWidget* parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;

 private slots:
    void pollUsb();

 private:
    QLabel* usbRow_ = nullptr;
    QLabel* sshRow_ = nullptr;
    QPushButton* connectByIpButton_ = nullptr;
    QTimer* poller_ = nullptr;
    bool usbOk_ = false;
    bool sshOk_ = false;

    void setRow(QLabel* row, bool ok, const QString& text);
};

}
