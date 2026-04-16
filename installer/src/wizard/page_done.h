#pragma once

#include <QString>
#include <QWizardPage>

class QLabel;
class QPushButton;
class QStackedWidget;

namespace rm::installer::wizard {

class PageDone : public QWizardPage {
    Q_OBJECT
 public:
    explicit PageDone(QWidget* parent = nullptr);

    void setSuccess(const QString& headingHtml, const QString& bodyHtml);
    void setFailure(const QString& errorText);

    void initializePage() override;

 signals:
    void retryRequested();
    void diagnosticsZipRequested();

 private:
    QStackedWidget* stack_ = nullptr;
    QLabel* successLabel_ = nullptr;
    QLabel* failureLabel_ = nullptr;
    QLabel* errorLineLabel_ = nullptr;
    QPushButton* retryButton_ = nullptr;
    QPushButton* diagnosticsButton_ = nullptr;
};

}
