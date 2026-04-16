#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QSysInfo>

class DiagRunner : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int fbWidth READ fbWidth NOTIFY diagnosticsComplete)
    Q_PROPERTY(int fbHeight READ fbHeight NOTIFY diagnosticsComplete)
    Q_PROPERTY(int fbStride READ fbStride NOTIFY diagnosticsComplete)
    Q_PROPERTY(int fbStridePixels READ fbStridePixels NOTIFY diagnosticsComplete)
    Q_PROPERTY(int fbBitsPerPixel READ fbBitsPerPixel NOTIFY diagnosticsComplete)
    Q_PROPERTY(QString fbId READ fbId NOTIFY diagnosticsComplete)
    Q_PROPERTY(float dpi READ dpi NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool strideMatchesWidth READ strideMatchesWidth NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool dpiAbove150 READ dpiAbove150 NOTIFY diagnosticsComplete)
    Q_PROPERTY(QString deviceModel READ deviceModel NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool allPassed READ allPassed NOTIFY diagnosticsComplete)
    Q_PROPERTY(QString resultsJson READ resultsJson NOTIFY diagnosticsComplete)

    Q_PROPERTY(bool sc1_displayCorrect READ sc1DisplayCorrect NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool sc2_edgeLineOk READ sc2EdgeLineOk NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool sc3_dpiCorrect READ sc3DpiCorrect NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool sc4_sleepWakeOk READ sc4SleepWakeOk NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool sc5_coexistenceOk READ sc5CoexistenceOk NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool allCriteriaPassed READ allCriteriaPassed NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool touchWorking READ touchWorking NOTIFY diagnosticsComplete)
    Q_PROPERTY(bool penDiscovered READ penDiscovered NOTIFY diagnosticsComplete)

public:
    explicit DiagRunner(QObject *parent = nullptr);

    int fbWidth() const { return m_fbWidth; }
    int fbHeight() const { return m_fbHeight; }
    int fbStride() const { return m_fbStride; }
    int fbStridePixels() const { return m_fbStridePixels; }
    int fbBitsPerPixel() const { return m_fbBitsPerPixel; }
    QString fbId() const { return m_fbId; }
    float dpi() const { return m_dpi; }
    bool strideMatchesWidth() const { return m_strideMatchesWidth; }
    bool dpiAbove150() const { return m_dpiAbove150; }
    QString deviceModel() const { return m_deviceModel; }
    bool allPassed() const { return m_allPassed; }
    QString resultsJson() const { return m_resultsJson; }

    Q_INVOKABLE void runAll();
    Q_INVOKABLE void exportJson(const QString &path);

    bool sc1DisplayCorrect() const { return m_sc1; }
    bool sc2EdgeLineOk() const { return m_sc2; }
    bool sc3DpiCorrect() const { return m_dpiAbove150; }
    bool sc4SleepWakeOk() const { return m_sc4; }
    bool sc5CoexistenceOk() const { return m_sc5; }
    bool allCriteriaPassed() const { return m_sc1 && m_sc2 && m_dpiAbove150 && m_sc4 && m_sc5; }
    bool touchWorking() const { return m_touchWorking; }
    bool penDiscovered() const { return m_penDiscovered; }

    Q_INVOKABLE void markEdgeLineVerified(bool ok);
    Q_INVOKABLE void markSleepWakeTested(bool ok);
    Q_INVOKABLE void markCoexistenceTested(bool ok);
    Q_INVOKABLE void markTouchWorking(bool ok);
    Q_INVOKABLE void markPenDiscovered(bool ok);
    Q_INVOKABLE void setPenAxisData(int maxX, int maxY, int maxPressure);
    Q_INVOKABLE void setInputDevices(const QVariantList &devices);

signals:
    void diagnosticsComplete();

private:
    void probeDevice();
    void probeFb();
    void probeDpi();
    bool detectIsMove() const;

    int m_fbWidth = 0;
    int m_fbHeight = 0;
    int m_fbStride = 0;
    int m_fbStridePixels = 0;
    int m_fbBitsPerPixel = 0;
    QString m_fbId;
    float m_dpi = 0.0f;
    bool m_strideMatchesWidth = false;
    bool m_dpiAbove150 = false;
    QString m_deviceModel;
    bool m_allPassed = false;
    QString m_resultsJson;

    bool m_sc1 = false;
    bool m_sc2 = false;
    bool m_sc4 = false;
    bool m_sc5 = false;
    bool m_touchWorking = false;
    bool m_penDiscovered = false;
    int m_penMaxX = 0, m_penMaxY = 0, m_penMaxPressure = 0;
    QVariantList m_inputDevices;
};
