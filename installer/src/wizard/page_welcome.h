#pragma once

#include <QWizardPage>

namespace rm::installer::wizard {

class PageWelcome : public QWizardPage {
    Q_OBJECT
 public:
    explicit PageWelcome(QWidget* parent = nullptr);
};

}
