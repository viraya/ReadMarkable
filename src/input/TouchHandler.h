#pragma once

#include <QObject>
#include <QPointF>
#include <QElapsedTimer>

class TouchHandler : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal lastX READ lastX NOTIFY touchEvent)
    Q_PROPERTY(qreal lastY READ lastY NOTIFY touchEvent)
    Q_PROPERTY(int tapCount READ tapCount NOTIFY touchEvent)
    Q_PROPERTY(QString lastGesture READ lastGesture NOTIFY gestureDetected)

public:
    explicit TouchHandler(QObject *parent = nullptr);

    qreal lastX() const { return m_lastX; }
    qreal lastY() const { return m_lastY; }
    int tapCount() const { return m_tapCount; }
    QString lastGesture() const { return m_lastGesture; }

    Q_INVOKABLE void onTouchPressed(qreal x, qreal y);
    Q_INVOKABLE void onTouchReleased(qreal x, qreal y);
    Q_INVOKABLE void onTouchMoved(qreal x, qreal y);
    Q_INVOKABLE void reset();

signals:
    void touchEvent();
    void gestureDetected();

private:
    qreal m_lastX = 0;
    qreal m_lastY = 0;
    int m_tapCount = 0;
    QString m_lastGesture;
    QPointF m_pressPoint;
    QElapsedTimer m_pressTimer;
};
