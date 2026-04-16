#pragma once

#include <QString>

namespace rm::installer::diagnostics {

bool writeDiagnosticsArchive(const QString& sessionLogPath,
                             const QString& lastDeviceStateJson,
                             const QString& destArchivePath,
                             QString* err = nullptr);

}
