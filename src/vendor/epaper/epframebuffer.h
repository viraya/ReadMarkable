#pragma once
// EPFrameBuffer compatibility header for reMarkable e-ink display control.
//
// The actual display API differs between device generations:
//   - Paper Pro (ferrari): may use fb0 + MXCFB ioctls (legacy approach)
//   - Paper Pro Move (chiappa): DRM-based via libqsgepaper.so scene graph plugin
//     Real class: EPFramebuffer (lowercase b) with swapBuffers() API
//     Backends: EPFramebufferAcep2 (Gallery 3), EPFramebufferSwtcon
//
// This header provides a static API that:
//   1. At runtime, tries to load EPFramebuffer from the scene graph plugin
//   2. Falls back to QPlatformNativeInterface window property hints
//   3. Falls back to no-op (QPA handles waveforms automatically)
//
// The QPA scene graph plugin (libqsgepaper.so) already manages waveform
// selection based on content type. Explicit waveform control is only needed
// for overriding the default behavior (e.g., forcing a full GC16 refresh).

#include <QRect>
#include <QImage>

class EPFrameBuffer {
public:
    // Waveform modes, conceptual mapping to what the QPA understands
    // The actual values may not match the QPA's internal enum
    enum WaveformMode {
        Initialize          = 0,   // Full init waveform (slow, full quality)
        Mono                = 1,   // DU waveform, 1-bit black/white, fastest
        DU                  = 1,   // Alias for Mono
        HighQualityGrayscale = 2,  // GC16, 16-level grayscale, high quality
        GC16                = 2,   // Alias for HighQualityGrayscale
        Grayscale           = 3,   // GL16, 16-level grayscale, lower quality
        GL16                = 3,   // Alias for Grayscale
    };

    // Update modes, partial vs full panel refresh
    enum UpdateMode {
        PartialUpdate = 0x0,  // Update only the specified rect
        FullUpdate    = 0x1,  // Full panel refresh (eliminates ghosting)
    };

    // Request a display update for a screen region.
    // On DRM devices (chiappa), this is a hint to the scene graph plugin.
    // On fbdev devices (ferrari), this would trigger MXCFB ioctls.
    static void sendUpdate(QRect rect,
                           WaveformMode waveform,
                           UpdateMode mode,
                           bool sync = false);

    // Clear the entire screen to white using a full Initialize waveform.
    static void clearScreen();

    // Block until the last submitted update has completed on the panel.
    static void waitForLastUpdate();

    // Return a pointer to the raw framebuffer as a QImage.
    // Returns nullptr on DRM devices where direct fb access isn't available.
    static QImage *framebuffer();

    // Check if the real EPFramebuffer (from libqsgepaper.so) is available
    static bool isAvailable();
};
