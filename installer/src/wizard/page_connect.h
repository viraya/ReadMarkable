#pragma once

#include <QString>
#include <QWizardPage>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace rm::installer::wizard {

class PageConnect : public QWizardPage {
    Q_OBJECT
 public:
    explicit PageConnect(QWidget* parent = nullptr);

    bool isComplete() const override;
    bool validatePage() override;
    int nextId() const override;

 private:
    QLineEdit* ipEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QCheckBox* rememberCheck_ = nullptr;
    QPushButton* findPasswordButton_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    void showStatus(const QString& message, bool isError);
};

}
