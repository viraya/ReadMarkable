#include <QtTest/QtTest>

extern "C" {
#include "on-device/src/xovi-trigger/gesture.h"
}

class TestGesture : public QObject {
    Q_OBJECT
 private slots:
    void resetClearsState();
    void singleTapDoesNotFire();
    void twoTapsDoNotFire();
    void threeTapsWithinWindowFire();
    void threeTapsOutsideWindowDoNotFire();
    void slidingWindowFiresOnFourthTapWithinRange();
    void disarmBlocksFurtherFires();
    void hasFiredReflectsState();
};

void TestGesture::resetClearsState() {
    gesture_t g;
    gesture_reset(&g);
    QCOMPARE(g.count, 0);
    QCOMPARE(g.fired, 0);
}

void TestGesture::singleTapDoesNotFire() {
    gesture_t g;
    gesture_reset(&g);
    QCOMPARE(gesture_record(&g, 1000), 0);
    QCOMPARE(gesture_has_fired(&g), 0);
}

void TestGesture::twoTapsDoNotFire() {
    gesture_t g;
    gesture_reset(&g);
    QCOMPARE(gesture_record(&g, 1000), 0);
    QCOMPARE(gesture_record(&g, 1500), 0);
    QCOMPARE(gesture_has_fired(&g), 0);
}

void TestGesture::threeTapsWithinWindowFire() {
    gesture_t g;
    gesture_reset(&g);
    QCOMPARE(gesture_record(&g, 1000), 0);
    QCOMPARE(gesture_record(&g, 1500), 0);
    QCOMPARE(gesture_record(&g, 2500), 1);
    QCOMPARE(gesture_has_fired(&g), 1);
}

void TestGesture::threeTapsOutsideWindowDoNotFire() {
    gesture_t g;
    gesture_reset(&g);
    QCOMPARE(gesture_record(&g, 1000), 0);
    QCOMPARE(gesture_record(&g, 1500), 0);
    QCOMPARE(gesture_record(&g, 3100), 0);
    QCOMPARE(gesture_has_fired(&g), 0);
}

void TestGesture::slidingWindowFiresOnFourthTapWithinRange() {
    gesture_t g;
    gesture_reset(&g);
    QCOMPARE(gesture_record(&g, 0), 0);
    QCOMPARE(gesture_record(&g, 1001), 0);
    QCOMPARE(gesture_record(&g, 2502), 0);
    QCOMPARE(gesture_record(&g, 3000), 1);
    QCOMPARE(gesture_has_fired(&g), 1);
}

void TestGesture::disarmBlocksFurtherFires() {
    gesture_t g;
    gesture_reset(&g);
    gesture_record(&g, 1000);
    gesture_record(&g, 1500);
    QCOMPARE(gesture_record(&g, 2500), 1);
    gesture_disarm(&g);
    QCOMPARE(gesture_record(&g, 3000), 0);
    QCOMPARE(gesture_record(&g, 3500), 0);
    QCOMPARE(gesture_record(&g, 4200), 0);
    QCOMPARE(gesture_has_fired(&g), 1);
}

void TestGesture::hasFiredReflectsState() {
    gesture_t g;
    gesture_reset(&g);
    QCOMPARE(gesture_has_fired(&g), 0);
    gesture_record(&g, 100);
    gesture_record(&g, 200);
    gesture_record(&g, 300);
    QCOMPARE(gesture_has_fired(&g), 1);
}

QTEST_MAIN(TestGesture)
#include "test_gesture.moc"
