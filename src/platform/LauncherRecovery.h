#pragma once

#include <QString>

class LauncherRecovery
{
public:
    static void run(const QString &installRoot);

private:
    LauncherRecovery() = delete;
};
