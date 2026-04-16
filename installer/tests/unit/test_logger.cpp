#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "diagnostics/logger.h"

using rm::installer::diagnostics::Logger;

class TestLogger : public QObject {
    Q_OBJECT
 private slots:
    void initTestCase();
    void writesSessionMarkers();
    void persistsAcrossInstances();

 private:
    QTemporaryDir tempCache_;
};

void TestLogger::initTestCase() {
    QVERIFY(tempCache_.isValid());
    // Redirect QStandardPaths::CacheLocation to our temp dir.
    QStandardPaths::setTestModeEnabled(true);
    qputenv("XDG_CACHE_HOME", tempCache_.path().toUtf8());
}

void TestLogger::writesSessionMarkers() {
    auto log = Logger::openForFlow(QStringLiteral("install"));
    QVERIFY(log != nullptr);
    QVERIFY(QFile::exists(log->filePath()));
    log->writePhaseMarker(QStringLiteral("probe"));
    log->write(QStringLiteral("stdout line 1"));
    log->writeError(QStringLiteral("stderr line 1"));
    const QString path = log->filePath();
    log.reset();  // closes file + writes session-end

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray content = f.readAll();
    QVERIFY(content.contains("session start"));
    QVERIFY(content.contains("flow=install"));
    QVERIFY(content.contains("=== phase: probe ==="));
    QVERIFY(content.contains("stdout line 1"));
    QVERIFY(content.contains("[ERROR] stderr line 1"));
    QVERIFY(content.contains("session end"));
}

void TestLogger::persistsAcrossInstances() {
    auto a = Logger::openForFlow(QStringLiteral("first"));
    QVERIFY(a != nullptr);
    const QString ap = a->filePath();
    a.reset();

    auto b = Logger::openForFlow(QStringLiteral("second"));
    QVERIFY(b != nullptr);
    QVERIFY(b->filePath() != ap);
    QVERIFY(QFile::exists(ap));
    QVERIFY(QFile::exists(b->filePath()));
}

QTEST_MAIN(TestLogger)
#include "test_logger.moc"
