#pragma once

#include <QObject>
#include <QString>

class QQuickWindow;
class LibraryModel;
class LibrarySortProxy;

class DebugController : public QObject
{
    Q_OBJECT

public:
    explicit DebugController(LibraryModel *libraryModel,
                             LibrarySortProxy *libraryProxy,
                             LibrarySortProxy *folderProxy,
                             LibrarySortProxy *bookProxy,
                             QObject *parent = nullptr);

    void setWindow(QQuickWindow *window);

    Q_INVOKABLE QString takeScreenshot(const QString &name = QString());

    Q_INVOKABLE QString libraryStats();

    Q_INVOKABLE void quitApp();

signals:

    void aboutToQuit();

private:
    QQuickWindow      *m_window       = nullptr;
    LibraryModel      *m_libraryModel = nullptr;
    LibrarySortProxy  *m_libraryProxy = nullptr;
    LibrarySortProxy  *m_folderProxy  = nullptr;
    LibrarySortProxy  *m_bookProxy    = nullptr;
};
