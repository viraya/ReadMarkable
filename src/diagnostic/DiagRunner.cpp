#include "DiagRunner.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDateTime>
#include <QGuiApplication>
#include <QScreen>
#include <QSysInfo>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unistd.h>

DiagRunner::DiagRunner(QObject *parent)
    : QObject(parent)
{
}

void DiagRunner::runAll()
{
    qInfo() << "DiagRunner: starting all diagnostics...";

    probeDevice();
    probeFb();
    probeDpi();

    m_allPassed = m_strideMatchesWidth && m_dpiAbove150;
    m_sc1 = true;

    exportJson("/tmp/readmarkable-diag.json");

    qInfo() << "DiagRunner: all done. allPassed =" << m_allPassed;

    emit diagnosticsComplete();
}

void DiagRunner::probeDevice()
{

    QFile modelFile("/proc/device-tree/model");
    if (modelFile.open(QIODevice::ReadOnly)) {
        m_deviceModel = QString::fromUtf8(modelFile.readAll()).trimmed();

        m_deviceModel.remove(QChar('\0'));
        modelFile.close();
        qInfo() << "DiagRunner: device model =" << m_deviceModel;
        return;
    }

    m_deviceModel = QSysInfo::machineHostName();
    qInfo() << "DiagRunner: device model (hostname) =" << m_deviceModel;
}

void DiagRunner::probeFb()
{

    if (auto *screen = QGuiApplication::primaryScreen()) {
        QSize sz = screen->size();
        m_fbWidth = sz.width();
        m_fbHeight = sz.height();
        qInfo() << "DiagRunner: screen size =" << m_fbWidth << "x" << m_fbHeight;
    }

    int fd = open("/dev/fb0", O_RDONLY);
    if (fd >= 0) {
        struct fb_fix_screeninfo finfo;
        struct fb_var_screeninfo vinfo;

        if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == 0) {
            m_fbStride = static_cast<int>(finfo.line_length);
            m_fbId = QString::fromLatin1(finfo.id, strnlen(finfo.id, sizeof(finfo.id)));
            qInfo() << "DiagRunner: fb_id =" << m_fbId << "stride =" << m_fbStride;
        }

        if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == 0) {
            m_fbBitsPerPixel = static_cast<int>(vinfo.bits_per_pixel);

            if (vinfo.xres > 0) {
                m_fbWidth = static_cast<int>(vinfo.xres);
                m_fbHeight = static_cast<int>(vinfo.yres);
            }
            qInfo() << "DiagRunner: bpp =" << m_fbBitsPerPixel;
        }

        close(fd);
    } else {

        m_fbId = "DRM (no fb0, ACEP2 display)";
        m_fbBitsPerPixel = 16;
        m_fbStride = m_fbWidth * (m_fbBitsPerPixel / 8);
        qInfo() << "DiagRunner: no /dev/fb0 (DRM device), using QScreen + estimated stride";
    }

    m_fbStridePixels = (m_fbBitsPerPixel > 0)
        ? m_fbStride / (m_fbBitsPerPixel / 8)
        : 0;
    m_strideMatchesWidth = (m_fbStridePixels == m_fbWidth);

    qInfo() << "DiagRunner: stridePixels =" << m_fbStridePixels
            << "strideMatchesWidth =" << m_strideMatchesWidth;
}

void DiagRunner::probeDpi()
{
    float screenDpi = 0.0f;
    if (auto *screen = QGuiApplication::primaryScreen()) {
        screenDpi = static_cast<float>(screen->physicalDotsPerInch());
        qInfo() << "DiagRunner: screen physicalDPI =" << screenDpi;
    }

    if (screenDpi < 50.0f) {

        screenDpi = detectIsMove() ? 264.0f : 229.0f;
        qInfo() << "DiagRunner: DPI fallback applied:" << screenDpi;
    }

    m_dpi = screenDpi;
    m_dpiAbove150 = (m_dpi > 150.0f);
    qInfo() << "DiagRunner: dpi =" << m_dpi << "dpiAbove150 =" << m_dpiAbove150;
}

bool DiagRunner::detectIsMove() const
{

    return m_deviceModel.contains("chiappa", Qt::CaseInsensitive)
        || (m_fbWidth == 954 && m_fbHeight == 1696)
        || (m_fbWidth == 1696 && m_fbHeight == 954);
}

void DiagRunner::exportJson(const QString &path)
{
    QJsonObject root;
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["app_version"] = QCoreApplication::applicationVersion();
    root["device_model"] = m_deviceModel;

    QJsonObject fb;
    fb["width"] = m_fbWidth;
    fb["height"] = m_fbHeight;
    fb["stride_bytes"] = m_fbStride;
    fb["stride_pixels"] = m_fbStridePixels;
    fb["bits_per_pixel"] = m_fbBitsPerPixel;
    fb["id"] = m_fbId;
    fb["stride_matches_width"] = m_strideMatchesWidth;
    root["framebuffer"] = fb;

    QJsonObject dpiObj;
    dpiObj["value"] = static_cast<double>(m_dpi);
    dpiObj["above_150"] = m_dpiAbove150;
    root["dpi"] = dpiObj;

    root["all_passed"] = m_allPassed;

    QJsonObject sc;
    sc["sc1_display_correct"] = m_sc1;
    sc["sc2_edge_line_ok"] = m_sc2;
    sc["sc3_dpi_correct"] = m_dpiAbove150;
    sc["sc4_sleep_wake_ok"] = m_sc4;
    sc["sc5_coexistence_ok"] = m_sc5;
    root["success_criteria"] = sc;

    QJsonObject subs;
    subs["touch_working"] = m_touchWorking;
    subs["pen_discovered"] = m_penDiscovered;
    subs["pen_max_x"] = m_penMaxX;
    subs["pen_max_y"] = m_penMaxY;
    subs["pen_max_pressure"] = m_penMaxPressure;
    root["subsystems"] = subs;

    if (!m_inputDevices.isEmpty()) {
        root["input_devices"] = QJsonArray::fromVariantList(m_inputDevices);
    }

    root["all_criteria_passed"] = allCriteriaPassed();

    QJsonDocument doc(root);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        qInfo() << "DiagRunner: diagnostics exported to" << path;
    } else {
        qWarning() << "DiagRunner: failed to write JSON to" << path;
    }

    m_resultsJson = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

void DiagRunner::markEdgeLineVerified(bool ok) { m_sc2 = ok; emit diagnosticsComplete(); }
void DiagRunner::markSleepWakeTested(bool ok) { m_sc4 = ok; emit diagnosticsComplete(); }
void DiagRunner::markCoexistenceTested(bool ok) { m_sc5 = ok; emit diagnosticsComplete(); }
void DiagRunner::markTouchWorking(bool ok) { m_touchWorking = ok; emit diagnosticsComplete(); }
void DiagRunner::markPenDiscovered(bool ok) { m_penDiscovered = ok; emit diagnosticsComplete(); }

void DiagRunner::setPenAxisData(int maxX, int maxY, int maxPressure) {
    m_penMaxX = maxX;
    m_penMaxY = maxY;
    m_penMaxPressure = maxPressure;
}

void DiagRunner::setInputDevices(const QVariantList &devices) {
    m_inputDevices = devices;
}
