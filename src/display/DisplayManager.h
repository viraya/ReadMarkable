#pragma once

#include <QObject>
#include <QRect>
#include <QVariantList>
#include "WaveformStrategy.h"

class DisplayManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(int screenWidth READ screenWidth CONSTANT)
    Q_PROPERTY(int screenHeight READ screenHeight CONSTANT)

public:
    explicit DisplayManager(QObject *parent = nullptr);

    int screenWidth() const;
    int screenHeight() const;
    QRect fullScreenRect() const;

    void refresh(const QRect &region, DisplayOperation op);
    void refreshFullScreen(DisplayOperation op);

    Q_INVOKABLE void pageTurn(const QRect &region);
    Q_INVOKABLE void fullClear();
    Q_INVOKABLE void initialize();

    Q_INVOKABLE QVariantList runWaveformTest();

signals:
    void refreshComplete(int op, int durationMs);

private:
    int m_width = 0;
    int m_height = 0;
};
