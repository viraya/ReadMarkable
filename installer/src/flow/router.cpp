#include "flow/router.h"

#include "core/pinned_versions.h"

namespace rm::installer::flow {

namespace {

constexpr const char* kTagUnknown = "unknown";
constexpr const char* kSourceInstaller = "readmarkable-installer";

int compareXoviAgainstMinimum(const QString& detectedTag,
                              const QString& minimumTag) {
    if (detectedTag == QLatin1String(kTagUnknown) || detectedTag.isEmpty()) {
        return 0;
    }
    return core::PinnedConfig::compareXoviTag(detectedTag, minimumTag);
}

}

RouterDecision decideInstallOrUpdate(const DeviceState& state,
                                     const core::PinnedConfig& pinned) {
    RouterDecision d;

    if (!state.isSupportedModel()) {
        d.variant = FlowVariant::BlockingError;
        d.blockingReason = BlockingReason::UnsupportedModel;
        d.summary = QStringLiteral("Unsupported device: %1").arg(state.deviceModel);
        d.errorDetails = QStringLiteral(
            "This installer supports reMarkable Paper Pro and Paper Pro Move. "
            "Detected: '%1'.").arg(state.deviceModel);
        return d;
    }

    const bool rmPresent = state.readmarkable.installed;
    const bool xoviPresent = state.xovi.installed;

    if (rmPresent && !xoviPresent) {
        d.variant = FlowVariant::BlockingError;
        d.blockingReason = BlockingReason::RmPresentNoXovi;
        d.summary = QStringLiteral(
            "ReadMarkable is installed without its XOVI runtime (corrupt state).");
        d.errorDetails = QStringLiteral(
            "ReadMarkable v%1 is installed but XOVI is missing. Uninstall "
            "ReadMarkable (via this installer), then run Install again.")
            .arg(state.readmarkable.version);
        return d;
    }

    int xoviCmp = 0;
    if (xoviPresent) {
        xoviCmp = compareXoviAgainstMinimum(state.xovi.tag,
                                            pinned.xovi.minimumCompatibleTag);
        if (xoviCmp < 0) {
            d.variant = FlowVariant::BlockingError;
            d.blockingReason = BlockingReason::XoviTooOld;
            d.summary = QStringLiteral(
                "XOVI %1 is older than ReadMarkable v%2 requires.")
                .arg(state.xovi.tag, pinned.readmarkable.minVersion);
            d.errorDetails = QStringLiteral(
                "Please update XOVI to %1 or newer, then re-run this installer.")
                .arg(pinned.xovi.minimumCompatibleTag);
            return d;
        }
    }

    if (xoviPresent) {
        const bool xoviVersionUnknown =
            (state.xovi.tag == QLatin1String(kTagUnknown) || state.xovi.tag.isEmpty());

        d.skipXoviInstall = true;
        if (xoviVersionUnknown) {

            d.skipAppLoadInstall = true;
        } else {

            if (state.appload.installed) {
                d.skipAppLoadInstall = true;
            } else {
                d.needsAppLoadInstall = true;
            }
        }
    }

    if (xoviPresent && !state.xovi.triggerInstalled) {
        d.offerBootPersistence = true;
    }

    if (rmPresent) {
        d.variant = FlowVariant::Update;

        QString rmDisplay = state.readmarkable.version;
        if (rmDisplay.startsWith(QLatin1Char('v')) ||
            rmDisplay.startsWith(QLatin1Char('V'))) {
            rmDisplay.remove(0, 1);
        }
        d.summary = QStringLiteral("ReadMarkable v%1 is installed. Update available.")
                        .arg(rmDisplay);
    } else {
        d.variant = FlowVariant::Install;
        if (xoviPresent) {
            d.summary = QStringLiteral(
                "XOVI %1 detected. Will install ReadMarkable only.")
                .arg(state.xovi.tag);
        } else {
            d.summary = QStringLiteral(
                "Fresh device. Will install XOVI + AppLoad + ReadMarkable.");
        }
    }
    return d;
}

RouterDecision decideUninstall(const DeviceState& state) {
    RouterDecision d;
    d.variant = FlowVariant::Uninstall;

    d.showRemoveXoviCheckbox =
        state.xovi.installed &&
        state.xovi.installSource == QLatin1String(kSourceInstaller);
    QString rmDisplay = state.readmarkable.version;
    if (rmDisplay.isEmpty()) {
        rmDisplay = QStringLiteral("(unknown)");
    } else if (rmDisplay.startsWith(QLatin1Char('v')) ||
               rmDisplay.startsWith(QLatin1Char('V'))) {
        rmDisplay.remove(0, 1);
    }
    d.summary = QStringLiteral("Will remove ReadMarkable v%1.").arg(rmDisplay);
    return d;
}

}
