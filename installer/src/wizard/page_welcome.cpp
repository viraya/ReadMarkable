#include "wizard/page_welcome.h"

#include <QLabel>
#include <QVBoxLayout>

#include "core/version.h"

namespace rm::installer::wizard {

PageWelcome::PageWelcome(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("ReadMarkable Installer"));
    setSubTitle(tr("One-click install of ReadMarkable on your reMarkable Paper Pro."));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 20, 40, 20);
    layout->setSpacing(18);

    auto* blurb = new QLabel(
        tr("<p>This installer will install <b>XOVI</b>, <b>AppLoad</b>, and "
           "<b>ReadMarkable</b> on your reMarkable Paper Pro or Paper Pro Move.</p>"
           "<p>Your device continues to work normally  -  reMarkable Cloud sync, "
           "notebooks, and firmware updates are unaffected.</p>"),
        this);
    blurb->setWordWrap(true);
    layout->addWidget(blurb);

    auto* version = new QLabel(
        tr("<i>Installer version %1</i>").arg(rm::installer::kAppVersion), this);
    layout->addWidget(version, 0, Qt::AlignRight);

    layout->addStretch();
}

}
