#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QScreen>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QObject>
#include <QThread>
#include <QTimer>

#include <unistd.h>
#include <limits.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "diagnostic/DiagRunner.h"
#include "display/DisplayManager.h"
#include "input/InputDiscovery.h"
#include "input/TouchHandler.h"
#include "input/PenReader.h"
#include "platform/XochitlGuard.h"
#include "platform/LauncherRecovery.h"
#include "platform/LifecycleManager.h"
#include "epub/CrEngineRenderer.h"
#include "epub/PositionRepository.h"
#include "rendering/PageCache.h"
#include "rendering/LayoutThread.h"
#include "rendering/ReadingSettings.h"
#include "qml/ReaderImageProvider.h"
#include "settings/ReaderSettingsController.h"
#include "navigation/NavigationController.h"
#include "navigation/TocModel.h"
#include "navigation/BookmarkModel.h"
#include "navigation/BookmarkRepository.h"
#include "navigation/ReadingTimeEstimator.h"
#include "diagnostic/DebugController.h"
#include "library/LibraryModel.h"
#include "library/LibraryScanner.h"
#include "library/LibrarySortProxy.h"
#include "library/CoverImageProvider.h"
#include "library/XochitlLibrarySource.h"
#include "annotation/SelectionController.h"
#include "annotation/AnnotationService.h"
#include "annotation/AnnotationModel.h"
#include "annotation/MarginNoteService.h"
#include "dictionary/DictionaryService.h"
#include "input/PenInputController.h"
#include "search/SearchService.h"
#include "export/ExportService.h"

static void reexecIntoDetachedScope(int argc, char *argv[]) {
    if (getenv("READMARKABLE_DETACHED")) return;
    setenv("READMARKABLE_DETACHED", "1", 1);

    std::vector<std::string> args;
    args.reserve(argc + 8);
    args.emplace_back("systemd-run");
    args.emplace_back("--scope");
    args.emplace_back("--collect");
    args.emplace_back("--quiet");
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        args.emplace_back(std::string("--working-directory=") + cwd);
    }
    args.emplace_back("--");
    for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);

    std::vector<char*> c_args;
    c_args.reserve(args.size() + 1);
    for (auto &s : args) c_args.push_back(const_cast<char*>(s.c_str()));
    c_args.push_back(nullptr);

    execvp("systemd-run", c_args.data());
    fprintf(stderr, "readmarkable: systemd-run exec failed (%s); "
                    "continuing in current cgroup, AppLoad launches will freeze\n",
            strerror(errno));
    fflush(stderr);
}

int main(int argc, char *argv[]) {

    setenv("QT_QUICK_BACKEND", "epaper", 0);

    reexecIntoDetachedScope(argc, argv);

    QGuiApplication app(argc, argv);
    app.setApplicationName("ReadMarkable");
    app.setApplicationVersion("1.0.1");

    qInfo() << "ReadMarkable" << app.applicationVersion() << "starting...";
    qInfo() << "Qt version:" << qVersion();
    qInfo() << "Platform:" << app.platformName();

    {
        const QString fontsDir = QStringLiteral("/home/root/.readmarkable/fonts");
        const QFileInfoList ttfs = QDir(fontsDir).entryInfoList(
            QStringList() << QStringLiteral("*.ttf") << QStringLiteral("*.otf"),
            QDir::Files);
        int registered = 0;
        for (const QFileInfo &fi : ttfs) {
            const int id = QFontDatabase::addApplicationFont(fi.absoluteFilePath());
            if (id >= 0) ++registered;
        }
        qInfo() << "QFontDatabase: registered" << registered << "bundled fonts from" << fontsDir;
    }

    if (auto *screen = app.primaryScreen()) {
        qInfo() << "Screen size:" << screen->size();
        qInfo() << "Physical DPI:" << screen->physicalDotsPerInch();
        qInfo() << "Logical DPI:" << screen->logicalDotsPerInch();
        qInfo() << "Device pixel ratio:" << screen->devicePixelRatio();
    } else {
        qWarning() << "No primary screen detected";
    }

    XochitlGuard xochitlGuard(XochitlGuard::Strategy::SystemctlStopStart);

    xochitlGuard.waitForRelease();

    LauncherRecovery::run(QStringLiteral("/home/root/.local/share/readmarkable"));

    DiagRunner diagRunner;
    DisplayManager displayManager;
    LifecycleManager lifecycleManager(&displayManager);
    InputDiscovery inputDiscovery;
    TouchHandler touchHandler;
    PenReader penReader;

    PageCache                 pageCache(5);
    CrEngineRenderer          renderer;
    LayoutThread              layoutThread(&pageCache);
    ReaderSettingsController  settingsCtrl(&renderer, &layoutThread, &pageCache);

    qInfo() << "CrEngineRenderer initialized";

    BookmarkRepository       bookmarkRepo;
    NavigationController     navController(&renderer, &bookmarkRepo);
    ReadingTimeEstimator     readingTimeEstimator;

    AnnotationService        annotationService;

    SelectionController      selectionController(&renderer, &annotationService);

    MarginNoteService marginNoteService;
    marginNoteService.setRenderer(&renderer);

    PenInputController penInputController;
    penInputController.setDependencies(&penReader, &selectionController,
                                       &annotationService, &marginNoteService);

    QObject::connect(&penReader, &PenReader::penEvent,
                     &penInputController, &PenInputController::onPenEvent,
                     Qt::QueuedConnection);

    QObject::connect(&penInputController, &PenInputController::marginNoteStrokeCommitted,
                     &marginNoteService,
                     [&marginNoteService](const QString &xptr, const QByteArray &data) {
                         marginNoteService.saveMarginNote(xptr, data);
                         marginNoteService.loadNotesForPage();
                     });

    DictionaryService        dictionaryService;

    SearchService            searchService;

    ExportService exportService(&annotationService, &marginNoteService,
                                &bookmarkRepo, navController.tocModel());

    AnnotationModel          annotationModel;

    auto refreshAnnotationModel = [&annotationModel, &annotationService, &marginNoteService]() {
        annotationModel.refresh(&annotationService, &marginNoteService);
    };

    QObject::connect(&annotationService, &AnnotationService::annotationsChanged,
                     refreshAnnotationModel);
    QObject::connect(&marginNoteService, &MarginNoteService::marginNotesChanged,
                     refreshAnnotationModel);

    PositionRepository positionRepo;
    QString currentBookPath;

    QObject::connect(&renderer, &CrEngineRenderer::loadedChanged, [&]() {
        if (!renderer.isLoaded()) return;
        qInfo() << "Document loaded:" << renderer.pageCount() << "pages";

        currentBookPath = renderer.documentPath();

        if (!currentBookPath.isEmpty()) {
            const QString saved = positionRepo.loadPosition(currentBookPath);
            if (!saved.isEmpty()) {
                renderer.goToXPointer(saved);
                qInfo() << "PositionRepository: restored position for"
                        << currentBookPath << "->" << saved;
            }
        }

        settingsCtrl.loadSettingsForBook(currentBookPath);

        navController.onDocumentLoaded(currentBookPath);

        annotationService.setCurrentBook(currentBookPath);

        marginNoteService.setCurrentBook(currentBookPath);

        annotationModel.refresh(&annotationService, &marginNoteService);

        searchService.setCurrentBook(currentBookPath);

        selectionController.loadHighlightsForPage();

        readingTimeEstimator.computeChapterWordCounts(renderer.docView());

        const int page = renderer.currentPage();
        QImage firstPage = renderer.renderPage(page);
        if (!firstPage.isNull()) {
            pageCache.insert(page, firstPage);
            qInfo() << "Seeded cache with page" << page;
        }

        renderer.bumpRenderVersion();

        qInfo() << "LayoutThread disabled, using synchronous rendering";
    });

    QObject::connect(&renderer, &CrEngineRenderer::pageCountChanged, [&]() {
        if (!renderer.isLoaded() || currentBookPath.isEmpty())
            return;
        qInfo() << "pageCountChanged: rebuilding search index for"
                << renderer.pageCount() << "pages";
        searchService.setCurrentBook(currentBookPath);
        searchService.buildIndex(&renderer, navController.tocModel());
    });

    QObject::connect(&renderer, &CrEngineRenderer::currentPageChanged, [&]() {
        if (!renderer.isLoaded()) return;
        const int page = renderer.currentPage();

        selectionController.clearSelection();

        selectionController.loadHighlightsForPage();

        marginNoteService.loadNotesForPage();

        if (!pageCache.contains(page)) {
            QImage img = renderer.renderPage(page);
            if (!img.isNull()) {
                pageCache.insert(page, img);
            }
        }
        renderer.bumpRenderVersion();

        if (!currentBookPath.isEmpty()) {
            const QString xptr = renderer.currentXPointer();
            if (!xptr.isEmpty()) {
                positionRepo.saveProgress(currentBookPath, xptr,
                                          navController.progressPercent(),
                                          navController.currentChapterTitle());
            }
        }

        layoutThread.setCurrentPage(page);
        layoutThread.requestPrerender(page);
    });

    QObject::connect(&renderer, &CrEngineRenderer::renderVersionChanged,
        [&displayManager](void) {
            displayManager.pageTurn(displayManager.fullScreenRect());
        }
    );

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        if (renderer.isLoaded() && !currentBookPath.isEmpty()) {
            const QString xptr = renderer.currentXPointer();
            if (!xptr.isEmpty()) {
                positionRepo.saveProgress(currentBookPath, xptr,
                                          navController.progressPercent(),
                                          navController.currentChapterTitle());
                qInfo() << "PositionRepository: saved final position on quit";
            }
        }
    });

    LibraryModel libraryModel;
    libraryModel.setPositionRepository(&positionRepo);
    LibrarySortProxy librarySortProxy;
    librarySortProxy.setSourceModel(&libraryModel);

    LibrarySortProxy folderSortProxy;
    folderSortProxy.setSourceModel(&libraryModel);
    folderSortProxy.setFilterMode(LibrarySortProxy::FoldersOnly);

    LibrarySortProxy bookSortProxy;
    bookSortProxy.setSourceModel(&libraryModel);
    bookSortProxy.setFilterMode(LibrarySortProxy::BooksOnly);

    QObject::connect(&librarySortProxy, &LibrarySortProxy::sortModeChanged,
                     [&folderSortProxy, &bookSortProxy, &librarySortProxy]() {
        const int mode = librarySortProxy.sortMode();
        folderSortProxy.setSortMode(mode);
        bookSortProxy.setSortMode(mode);
    });

    const QString libraryPath = QStringLiteral("/home/root/.local/share/readmarkable/books");
    LibraryScanner *libraryScanner = new LibraryScanner(libraryPath, &positionRepo);
    QThread *scanThread = new QThread;
    libraryScanner->moveToThread(scanThread);

    QObject::connect(libraryScanner, &LibraryScanner::bookFound,
                     &libraryModel,  &LibraryModel::addBook,
                     Qt::QueuedConnection);
    QObject::connect(libraryScanner, &LibraryScanner::scanComplete,
                     &libraryModel,  &LibraryModel::onScanComplete,
                     Qt::QueuedConnection);
    QObject::connect(scanThread, &QThread::started,
                     libraryScanner, &LibraryScanner::scan);

    QObject::connect(&libraryModel, &LibraryModel::rescanRequested,
                     libraryScanner, &LibraryScanner::scan,
                     Qt::QueuedConnection);
    QObject::connect(&libraryModel, &LibraryModel::rescanDirectoryRequested,
                     libraryScanner, &LibraryScanner::scanDirectory,
                     Qt::QueuedConnection);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        scanThread->quit();
        scanThread->wait(3000);
        delete libraryScanner;
        delete scanThread;
    });

    QObject::connect(&renderer, &CrEngineRenderer::loadedChanged, [&]() {
        if (renderer.isLoaded()) {
            libraryModel.setCurrentlyOpenBookPath(renderer.documentPath());
        } else {

            libraryModel.setCurrentlyOpenBookPath(QString());
        }
    });

    const QString xochitlDir = QStringLiteral("/home/root/.local/share/remarkable/xochitl");
    XochitlLibrarySource *xochitlSource = new XochitlLibrarySource(xochitlDir, &app);

    libraryModel.setXochitlSource(xochitlSource);

    QObject::connect(xochitlSource, &XochitlLibrarySource::entryAdded,
                     &libraryModel, &LibraryModel::onXochitlEntryAdded);
    QObject::connect(xochitlSource, &XochitlLibrarySource::entryRemoved,
                     &libraryModel, &LibraryModel::onXochitlEntryRemoved);
    QObject::connect(xochitlSource, &XochitlLibrarySource::scanComplete,
                     &libraryModel, &LibraryModel::onXochitlScanComplete);

    QQuickView view;

    auto onOrientationChanged = [&]() {
        const int newW = renderer.isLandscape() ? 1696 : 954;
        const int newH = renderer.isLandscape() ? 954  : 1696;

        qInfo() << "main: orientation changed to"
                << (renderer.isLandscape() ? "landscape" : "portrait")
                << newW << "x" << newH;

        view.resize(newW, newH);

        ReadingSettings settings;
        settings.viewWidth  = newW;
        settings.viewHeight = newH;
        layoutThread.applySettings(settings);

        pageCache.invalidateAll();

        layoutThread.requestPrerender(renderer.currentPage());

        displayManager.fullClear();
    };

    QObject::connect(&renderer, &CrEngineRenderer::orientationChanged,
                     &app, onOrientationChanged);

    view.engine()->addImportPath(QStringLiteral("qrc:/"));

    view.engine()->addImageProvider(
        QStringLiteral("reader"),
        new ReaderImageProvider(&pageCache)
    );

    view.engine()->addImageProvider(
        QStringLiteral("cover"),
        new CoverImageProvider
    );

    inputDiscovery.discover();
    if (inputDiscovery.penFound()) {
        penReader.start(inputDiscovery.penDevicePath());
        qInfo() << "PenReader started on" << inputDiscovery.penDevicePath();
    } else {
        qWarning() << "PenReader: no pen device found, pen input disabled";
    }

    view.rootContext()->setContextProperty("xochitlGuard",       &xochitlGuard);
    view.rootContext()->setContextProperty("diagRunner",         &diagRunner);
    view.rootContext()->setContextProperty("displayManager",     &displayManager);
    view.rootContext()->setContextProperty("lifecycleManager",   &lifecycleManager);
    view.rootContext()->setContextProperty("inputDiscovery",     &inputDiscovery);
    view.rootContext()->setContextProperty("touchHandler",       &touchHandler);
    view.rootContext()->setContextProperty("penReader",          &penReader);
    view.rootContext()->setContextProperty("renderer",           &renderer);
    view.rootContext()->setContextProperty("positionRepo",       &positionRepo);

    view.rootContext()->setContextProperty("settingsController", &settingsCtrl);

    view.rootContext()->setContextProperty("navController",  &navController);
    view.rootContext()->setContextProperty("tocModel",       navController.tocModel());
    view.rootContext()->setContextProperty("bookmarkModel",  navController.bookmarkModel());

    view.rootContext()->setContextProperty("libraryModel",      &libraryModel);
    view.rootContext()->setContextProperty("librarySortProxy",  &librarySortProxy);
    view.rootContext()->setContextProperty("folderSortProxy",   &folderSortProxy);
    view.rootContext()->setContextProperty("bookSortProxy",     &bookSortProxy);

    DebugController debugCtrl(&libraryModel, &librarySortProxy,
                              &folderSortProxy, &bookSortProxy);
    debugCtrl.setWindow(&view);
    view.rootContext()->setContextProperty("debugCtrl",         &debugCtrl);

    view.rootContext()->setContextProperty("selectionController", &selectionController);

    view.rootContext()->setContextProperty("annotationService",   &annotationService);

    view.rootContext()->setContextProperty("annotationModel",     &annotationModel);

    view.rootContext()->setContextProperty("dictionaryService",   &dictionaryService);

    view.rootContext()->setContextProperty("penInputController",  &penInputController);

    view.rootContext()->setContextProperty("marginNoteService",   &marginNoteService);

    view.rootContext()->setContextProperty("searchService",        &searchService);

    view.rootContext()->setContextProperty("exportService",        &exportService);

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl(QStringLiteral("qrc:/ReadMarkable/src/qml/Main.qml")));

    if (view.status() == QQuickView::Error) {
        qCritical() << "Failed to load QML:" << view.errors();
        return -1;
    }

    view.showFullScreen();

    QTimer::singleShot(500, &app, [&displayManager]() {
        qInfo() << "Startup: forcing initial full-panel refresh (fullClear)";
        displayManager.fullClear();
    });

    scanThread->start();

    xochitlSource->startWatching();

    qInfo() << "ReadMarkable running. Library scanner + xochitl watcher started.";
    return app.exec();
}
