#include "TouchHandler.h"

#include <QDebug>
#include <cmath>

TouchHandler::TouchHandler(QObject *parent) : QObject(parent) {}

void TouchHandler::onTouchPressed(qreal x, qreal y)
{
    m_lastX = x;
    m_lastY = y;
    m_pressPoint = QPointF(x, y);
    m_pressTimer.start();
    m_tapCount++;

    qInfo() << "TOUCH pressed at" << x << y << "tap#" << m_tapCount;
    emit touchEvent();
}

void TouchHandler::onTouchReleased(qreal x, qreal y)
{
    m_lastX = x;
    m_lastY = y;

    qreal dx = x - m_pressPoint.x();
    qreal dy = y - m_pressPoint.y();
    qreal dist = std::sqrt(dx * dx + dy * dy);
    qint64 elapsed = m_pressTimer.elapsed();

    if (dist < 20 && elapsed < 300) {
        m_lastGesture = "tap";
    } else if (std::abs(dx) > 100 && std::abs(dx) > std::abs(dy)) {
        m_lastGesture = (dx > 0) ? "swipe-right" : "swipe-left";
    } else if (std::abs(dy) > 100) {
        m_lastGesture = (dy > 0) ? "swipe-down" : "swipe-up";
    } else {
        m_lastGesture = "drag";
    }

    qInfo() << "TOUCH released at" << x << y
            << "gesture:" << m_lastGesture
            << "dist:" << dist << "ms:" << elapsed;

    emit touchEvent();
    emit gestureDetected();
}

void TouchHandler::onTouchMoved(qreal x, qreal y)
{
    m_lastX = x;
    m_lastY = y;
    emit touchEvent();
}

void TouchHandler::reset()
{
    m_lastX = 0;
    m_lastY = 0;
    m_tapCount = 0;
    m_lastGesture.clear();
    emit touchEvent();
}
