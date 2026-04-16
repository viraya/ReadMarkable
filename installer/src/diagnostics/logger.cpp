#include "diagnostics/logger.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

namespace rm::installer::diagnostics {

namespace {

QString logsDir() {
    const QString base = QStandardPaths::writableLocation(
        QStandardPaths::CacheLocation);
    if (base.isEmpty()) return {};
    const QString d = base + QStringLiteral("/logs");
    QDir().mkpath(d);
    return d;
}

QString isoStamp() {

    return QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmssZ"));
}

}

std::unique_ptr<Logger> Logger::openForFlow(const QString& flowName) {
    const QString dir = logsDir();
    if (dir.isEmpty()) return nullptr;

    auto self = std::unique_ptr<Logger>(new Logger());
    self->filePath_ = dir + QStringLiteral("/%1-%2.log").arg(isoStamp(), flowName);
    self->file_ = std::make_unique<QFile>(self->filePath_);
    if (!self->file_->open(QIODevice::WriteOnly | QIODevice::Append |
                            QIODevice::Text)) {
        return nullptr;
    }
    self->write(QStringLiteral("=== session start: %1 flow=%2 ===")
                    .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
                         flowName));
    return self;
}

Logger::~Logger() {
    if (file_ && file_->isOpen()) {
        write(QStringLiteral("=== session end ==="));
        file_->close();
    }
}

void Logger::write(const QString& line) {
    if (!file_ || !file_->isOpen()) return;
    const QString stamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    QByteArray out;
    out.reserve(stamp.size() + line.size() + 4);
    out.append(stamp.toUtf8()).append(' ');
    out.append(line.toUtf8()).append('\n');
    file_->write(out);
    file_->flush();
}

void Logger::writePhaseMarker(const QString& phase) {
    write(QStringLiteral("=== phase: %1 ===").arg(phase));
}

void Logger::writeError(const QString& line) {
    write(QStringLiteral("[ERROR] ") + line);
}

QString Logger::filePath() const { return filePath_; }

void Logger::evict(int keep) {
    QDir d(logsDir());
    if (!d.exists()) return;
    const auto entries = d.entryInfoList(QStringList{QStringLiteral("*.log")},
                                          QDir::Files, QDir::Time);
    for (int i = keep; i < entries.size(); ++i) {
        QFile::remove(entries[i].absoluteFilePath());
    }
}

}
