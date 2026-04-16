#include "DebugController.h"

#include "library/LibraryModel.h"
#include "library/LibrarySortProxy.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QTimer>

DebugController::DebugController(LibraryModel *libraryModel,
                                 LibrarySortProxy *libraryProxy,
                                 LibrarySortProxy *folderProxy,
                                 LibrarySortProxy *bookProxy,
                                 QObject *parent)
    : QObject(parent)
    , m_libraryModel(libraryModel)
    , m_libraryProxy(libraryProxy)
    , m_folderProxy(folderProxy)
    , m_bookProxy(bookProxy)
{
}

void DebugController::setWindow(QQuickWindow *window)
{
    m_window = window;
}

QString DebugController::takeScreenshot(const QString &name)
{
    if (!m_window) {
        qWarning() << "DebugController::takeScreenshot  -  no window bound";
        return QString();
    }

    QString safeName = name;
    safeName.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]")),
                     QStringLiteral("_"));
    if (safeName.isEmpty()) {
        safeName = QDateTime::currentDateTimeUtc().toString(
            QStringLiteral("yyyyMMdd_hhmmsszzz"));
    }

    const QString path = QStringLiteral("/tmp/readmarkable_") + safeName +
                         QStringLiteral(".png");

    const QImage img = m_window->grabWindow();
    if (img.isNull()) {
        qWarning() << "DebugController::takeScreenshot  -  grabWindow returned null image";
        return QString();
    }

    if (!img.save(path)) {
        qWarning() << "DebugController::takeScreenshot  -  save failed:" << path;
        return QString();
    }

    qDebug() << "DebugController: screenshot saved  - " << path
             << img.width() << "x" << img.height();
    return path;
}

QString DebugController::libraryStats()
{
    const int libCount    = m_libraryProxy ? m_libraryProxy->rowCount() : -1;
    const int folderCount = m_folderProxy  ? m_folderProxy->rowCount()  : -1;
    const int bookCount   = m_bookProxy    ? m_bookProxy->rowCount()    : -1;

    const QString lastReadPath  = m_libraryModel ? m_libraryModel->lastReadBookPath()   : QString();
    const QString lastReadTitle = m_libraryModel ? m_libraryModel->lastReadBookTitle()  : QString();
    const int lastReadProgress  = m_libraryModel ? m_libraryModel->lastReadBookProgress() : -1;

    const QString summary =
        QStringLiteral("libraryProxy=%1 folderProxy=%2 bookProxy=%3 | lastRead=\"%4\" (%5%%) path=%6")
            .arg(libCount)
            .arg(folderCount)
            .arg(bookCount)
            .arg(lastReadTitle)
            .arg(lastReadProgress)
            .arg(lastReadPath.isEmpty() ? QStringLiteral("<empty>") : lastReadPath);

    qDebug().noquote() << "DebugController stats:" << summary;
    return summary;
}

void DebugController::quitApp()
{
    qDebug() << "DebugController: quitApp  -  showing Returning splash then exiting";
    emit aboutToQuit();

    QTimer::singleShot(2500, this, []() {
        qDebug() << "DebugController: QCoreApplication::quit()";
        QCoreApplication::quit();
    });
}
