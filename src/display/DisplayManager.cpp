#include "DisplayManager.h"

#include <epframebuffer.h>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>
#include <QVariantMap>

DisplayManager::DisplayManager(QObject *parent)
    : QObject(parent)
{

    if (auto *screen = QGuiApplication::primaryScreen()) {
        QSize sz = screen->size();
        m_width = sz.width();
        m_height = sz.height();
    }

    bool hasRealFB = EPFrameBuffer::isAvailable();
    qInfo() << "DisplayManager initialized:" << m_width << "x" << m_height
            << "real EPFramebuffer:" << hasRealFB;
}

int DisplayManager::screenWidth() const { return m_width; }
int DisplayManager::screenHeight() const { return m_height; }
QRect DisplayManager::fullScreenRect() const { return QRect(0, 0, m_width, m_height); }

void DisplayManager::refresh(const QRect &region, DisplayOperation op)
{
    auto params = WaveformStrategy::paramsFor(op);

    QElapsedTimer timer;
    timer.start();

    EPFrameBuffer::sendUpdate(region, params.waveform, params.updateMode, params.sync);

    if (auto *window = QGuiApplication::focusWindow()) {
        window->requestUpdate();
    }

    int elapsed = static_cast<int>(timer.elapsed());
    qDebug() << "Display refresh:" << static_cast<int>(op)
             << "region:" << region << "took:" << elapsed << "ms";
    emit refreshComplete(static_cast<int>(op), elapsed);
}

void DisplayManager::refreshFullScreen(DisplayOperation op)
{
    refresh(fullScreenRect(), op);
}

void DisplayManager::pageTurn(const QRect &region)
{
    refresh(region, DisplayOperation::PageTurn);
}

void DisplayManager::fullClear()
{
    refreshFullScreen(DisplayOperation::FullClear);
}

void DisplayManager::initialize()
{
    refreshFullScreen(DisplayOperation::Initialize);
}

QVariantList DisplayManager::runWaveformTest()
{
    QVariantList results;

    struct TestCase {
        DisplayOperation op;
        QString name;
    };

    QList<TestCase> tests = {
        {DisplayOperation::Initialize, "Initialize"},
        {DisplayOperation::PageTurn,   "PageTurn (Mono)"},
        {DisplayOperation::FullClear,  "FullClear (GC16)"},
        {DisplayOperation::UiOverlay,  "UiOverlay (Mono partial)"},
        {DisplayOperation::PenStroke,  "PenStroke (DU)"},
        {DisplayOperation::ImageRender, "ImageRender (HQ Grayscale)"},
    };

    for (const auto &test : tests) {
        auto params = WaveformStrategy::paramsFor(test.op);

        QElapsedTimer timer;
        timer.start();

        QRect region = (test.op == DisplayOperation::Initialize)
            ? fullScreenRect()
            : QRect(100, 100, 400, 400);

        EPFrameBuffer::sendUpdate(region, params.waveform, params.updateMode, true);

        int elapsed = static_cast<int>(timer.elapsed());

        QVariantMap result;
        result["name"] = test.name;
        result["duration_ms"] = elapsed;
        result["waveform"] = static_cast<int>(params.waveform);
        result["update_mode"] = static_cast<int>(params.updateMode);
        result["real_epfb"] = EPFrameBuffer::isAvailable();
        results.append(result);

        qInfo() << "Waveform test:" << test.name << "=" << elapsed << "ms"
                << "(real EPFramebuffer:" << EPFrameBuffer::isAvailable() << ")";
    }

    return results;
}
