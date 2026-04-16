#pragma once

#include <QString>
#include <QWizardPage>

class QCheckBox;
class QLabel;

namespace rm::installer::wizard {

class PageActionSummary : public QWizardPage {
    Q_OBJECT
 public:
    enum Variant {
        Install,
        Update,
        Uninstall,
        BlockingError,
    };

    explicit PageActionSummary(QWidget* parent = nullptr);

    void setVariant(Variant v, const QString& summaryHtml);
    void setRemoveXoviCheckboxVisible(bool visible);
    bool removeXoviChecked() const;

    void setBootPersistenceCheckboxVisible(bool visible);
    bool bootPersistenceChecked() const;

    void initializePage() override;
    bool isComplete() const override;
    int nextId() const override;

 private:
    QLabel* summaryLabel_ = nullptr;
    QCheckBox* removeXoviCheck_ = nullptr;
    QCheckBox* bootPersistenceCheck_ = nullptr;
    bool blocked_ = false;
};

}
