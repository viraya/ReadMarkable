#include "LauncherRecovery.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace {

constexpr auto kDraftDir  = "/opt/etc/draft";
constexpr auto kIconDir   = "/opt/etc/draft/icons";
constexpr auto kDraftFile = "/opt/etc/draft/readmarkable.draft";
constexpr auto kIconFile  = "/opt/etc/draft/icons/readmarkable.png";

void restoreIfMissing(const QString &dst,
                      const QString &dstDir,
                      const QString &src,
                      const char    *label)
{
    if (QFile::exists(dst)) {
        return;
    }
    if (!QFile::exists(src)) {
        qInfo() << "LauncherRecovery: skip" << label
                << " -  canonical source missing:" << src;
        return;
    }
    if (!QDir().mkpath(dstDir)) {
        qWarning() << "LauncherRecovery:" << label
                   << " -  failed to mkpath" << dstDir;
        return;
    }
    if (!QFile::copy(src, dst)) {
        qWarning() << "LauncherRecovery:" << label
                   << " -  QFile::copy failed:" << src << "->" << dst;
        return;
    }
    qInfo() << "LauncherRecovery: restored" << label << "->" << dst;
}

}

void LauncherRecovery::run(const QString &installRoot)
{
    const QString canonicalDraft = installRoot + QStringLiteral("/launcher/readmarkable.draft");
    const QString canonicalIcon  = installRoot + QStringLiteral("/launcher/icon.png");

    restoreIfMissing(QString::fromLatin1(kDraftFile),
                     QString::fromLatin1(kDraftDir),
                     canonicalDraft,
                     "Draft entry");
    restoreIfMissing(QString::fromLatin1(kIconFile),
                     QString::fromLatin1(kIconDir),
                     canonicalIcon,
                     "Draft icon");
}
