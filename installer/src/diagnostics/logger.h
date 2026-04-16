#pragma once

#include <QDir>
#include <QFile>
#include <QString>
#include <memory>

namespace rm::installer::diagnostics {

class Logger {
 public:

    static std::unique_ptr<Logger> openForFlow(const QString& flowName);

    ~Logger();

    void write(const QString& line);
    void writePhaseMarker(const QString& phase);
    void writeError(const QString& line);

    QString filePath() const;

    static void evict(int keep = 20);

 private:
    Logger() = default;
    QString filePath_;
    std::unique_ptr<QFile> file_;
};

}
