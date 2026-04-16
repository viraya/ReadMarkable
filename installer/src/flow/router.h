#pragma once

#include <QString>

#include "core/pinned_versions.h"
#include "flow/device_state.h"

namespace rm::installer::flow {

enum class FlowVariant {
    Install,
    Update,
    Uninstall,
    BlockingError,
};

enum class BlockingReason {
    None,
    UnsupportedModel,
    XoviTooOld,
    RmPresentNoXovi,
};

struct RouterDecision {
    FlowVariant variant = FlowVariant::Install;
    BlockingReason blockingReason = BlockingReason::None;

    bool skipXoviInstall = false;
    bool skipAppLoadInstall = false;
    bool needsAppLoadInstall = false;
    bool offerBootPersistence = false;

    bool showRemoveXoviCheckbox = false;

    QString summary;
    QString errorDetails;
};

RouterDecision decideInstallOrUpdate(const DeviceState& state,
                                     const core::PinnedConfig& pinned);

RouterDecision decideUninstall(const DeviceState& state);

}
