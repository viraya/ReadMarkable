#include <QApplication>

#include "core/version.h"
#include "wizard/installer_wizard.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(rm::installer::kAppName);
    QApplication::setApplicationVersion(rm::installer::kAppVersion);
    QApplication::setOrganizationName("ReadMarkable");
    QApplication::setOrganizationDomain("readmarkable.io");

    rm::installer::wizard::InstallerWizard wizard;
    wizard.show();
    return app.exec();
}
