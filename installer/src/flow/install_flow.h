#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

#include "flow/flow_runner.h"

namespace rm::installer::flow {

struct InstallFlowInputs {
    QString remoteStagingDir;
    QString localXoviTarball;
    QString localApploadZip;
    QString localRmTarball;
    QStringList localOnDeviceFiles;

    QStringList localHomeTileFiles;
    QString xoviTag;
    QString apploadTag;
    bool skipXoviInstall = false;
    bool skipAppLoadInstall = false;
    bool installPersistence = true;
};

QList<Phase> buildInstallFlow(const InstallFlowInputs& in);

struct UpdateFlowInputs {
    QString remoteStagingDir;
    QString localRmTarball;
    QString installRmScript;

    QString rebuildHashtabScript;

    QString installHomeTileScript;
    QStringList homeTileFiles;

    bool installPersistence = false;
    QStringList persistenceFiles;
};

QList<Phase> buildUpdateFlow(const UpdateFlowInputs& in);

struct UninstallFlowInputs {
    QString remoteStagingDir;
    QStringList localOnDeviceFiles;
    bool alsoRemoveXovi = false;
};

QList<Phase> buildUninstallFlow(const UninstallFlowInputs& in);

}
