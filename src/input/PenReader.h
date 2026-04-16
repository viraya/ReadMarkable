#pragma once

#include <QObject>
#include <QThread>
#include <atomic>

class PenReader : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal penX READ penX NOTIFY penEvent)
    Q_PROPERTY(qreal penY READ penY NOTIFY penEvent)
    Q_PROPERTY(int pressure READ pressure NOTIFY penEvent)
    Q_PROPERTY(int tiltX READ tiltX NOTIFY penEvent)
    Q_PROPERTY(int tiltY READ tiltY NOTIFY penEvent)
    Q_PROPERTY(bool penDown READ penDown NOTIFY penEvent)
    Q_PROPERTY(bool penInRange READ penInRange NOTIFY penEvent)
    Q_PROPERTY(bool isEraser READ isEraser NOTIFY penEvent)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(int maxPressure READ maxPressure CONSTANT)
    Q_PROPERTY(int maxX READ maxX CONSTANT)
    Q_PROPERTY(int maxY READ maxY CONSTANT)

public:
    explicit PenReader(QObject *parent = nullptr);
    ~PenReader();

    Q_INVOKABLE bool start(const QString &devicePath);
    Q_INVOKABLE void stop();

    qreal penX() const { return m_x; }
    qreal penY() const { return m_y; }
    int pressure() const { return m_pressure; }
    int tiltX() const { return m_tiltX; }
    int tiltY() const { return m_tiltY; }
    bool penDown() const { return m_penDown; }
    bool isEraser() const { return m_isEraser; }
    bool penInRange() const { return m_inRange; }
    bool active() const { return m_active; }
    int maxPressure() const { return m_maxPressure; }
    int maxX() const { return m_maxX; }
    int maxY() const { return m_maxY; }

signals:
    void penEvent();
    void activeChanged();
    void evdevInfo(const QString &info);

private:
    void readLoop(const QString &devicePath);
    void queryAbsInfo(int fd);

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_active{false};

    qreal m_x = 0, m_y = 0;
    int m_pressure = 0;
    int m_tiltX = 0, m_tiltY = 0;
    bool m_penDown = false;
    bool m_inRange = false;
    bool m_isEraser = false;

    int m_maxPressure = 4096;
    int m_maxX = 0, m_maxY = 0;
    int m_minX = 0, m_minY = 0;
};
