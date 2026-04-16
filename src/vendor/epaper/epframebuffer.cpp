#include "epframebuffer.h"

#include <QGuiApplication>
#include <QDebug>
#include <QWindow>
#include <dlfcn.h>

// Runtime EPFramebuffer adapter
//
// The reMarkable's scene graph plugin (libqsgepaper.so) provides EPFramebuffer
// as a QObject singleton. We dlopen it at runtime to get direct waveform control.
//
// Actual API (demangled from libqsgepaper.so symbols):
//   EPFramebuffer::instance() -> EPFramebuffer*
//   EPFramebuffer::swapBuffers(QRect, EPContentType, EPScreenMode, QFlags<UpdateFlag>)
//   EPFramebuffer::ghostControl(GhostControlMode)
//
// EPScreenMode values (from strings analysis):
//   Mono, FastGrayscale, Content, TEXT, TEXT_GLR, FAST, INIT, PMODE, TMODE
//
// EPContentType values:
//   Mono (B&W), Grayscale, Content (mixed)

// EPScreenMode enum values, discovered from libqsgepaper.so strings
// These map to waveform lookup tables on device (/usr/share/remarkable/ct33_*.bin)
// Order is speculative, we probe at runtime to verify
enum EPScreenMode {
    EP_Mono            = 0,   // B&W only, fastest, maps to ct33_fast.bin
    EP_FastGrayscale   = 1,   // Fast grayscale
    EP_Content         = 2,   // Normal content rendering
    EP_Text            = 3,   // Text rendering
    EP_TextGLR         = 4,   // Text ghost-less refresh
    EP_Fast            = 5,   // Fast mode
    EP_Init            = 6,   // Full initialize, slowest, full quality
    EP_PMode           = 7,   // P-mode (partial?)
    EP_TMode           = 8,   // T-mode (temperature?)
};

// EPContentType, content classification for waveform selection
enum EPContentType {
    EP_ContentMono      = 0,  // Pure B&W content
    EP_ContentGrayscale = 1,  // Grayscale content
    EP_ContentMixed     = 2,  // Mixed content
};

namespace {

struct RealEPFB {
    void *handle = nullptr;
    void *instance = nullptr;
    bool tried = false;
    bool available = false;

    // Function pointers resolved at runtime via dlsym
    using InstanceFn = void*(*)();

    // swapBuffers is a member function, on AArch64, 'this' is the first implicit param
    // EPFramebuffer::swapBuffers(QRect, EPContentType, EPScreenMode, QFlags<UpdateFlag>)
    // QRect is 16 bytes (4 ints), passed by value in registers on AArch64
    using SwapBuffersFn = void(*)(void* /*this*/, QRect, int /*contentType*/,
                                  int /*screenMode*/, int /*flags*/);

    // EPFramebuffer::ghostControl(GhostControlMode)
    using GhostControlFn = void(*)(void* /*this*/, int /*mode*/);

    InstanceFn getInstance = nullptr;
    SwapBuffersFn swapBuffers = nullptr;
    GhostControlFn ghostControl = nullptr;

    void tryLoad() {
        if (tried) return;
        tried = true;

        // Mangled symbol names from libqsgepaper.so
        static const char *symInstance =
            "_ZN13EPFramebuffer8instanceEv";
        static const char *symSwapBuffers =
            "_ZN13EPFramebuffer11swapBuffersE5QRect13EPContentType12EPScreenMode6QFlagsINS_10UpdateFlagEE";
        static const char *symGhostControl =
            "_ZN13EPFramebuffer12ghostControlENS_16GhostControlModeE";

        // Try loading from already-loaded plugin, then force-load
        static const char *pluginPaths[] = {
            nullptr,  // RTLD_DEFAULT first
            "libqsgepaper.so",
            "/usr/lib/plugins/scenegraph/libqsgepaper.so",
        };

        for (int i = 0; i < 3 && !getInstance; ++i) {
            void *h = nullptr;
            if (i == 0) {
                // Try global symbol table
                getInstance = reinterpret_cast<InstanceFn>(
                    dlsym(RTLD_DEFAULT, symInstance));
            } else {
                h = dlopen(pluginPaths[i], (i == 1) ? (RTLD_LAZY | RTLD_NOLOAD) : RTLD_LAZY);
                if (h) {
                    getInstance = reinterpret_cast<InstanceFn>(dlsym(h, symInstance));
                    if (getInstance) {
                        handle = h;
                    } else {
                        dlclose(h);
                    }
                }
            }
        }

        if (getInstance) {
            instance = getInstance();
            available = (instance != nullptr);

            // Resolve additional methods from the same handle
            void *h = handle ? handle : RTLD_DEFAULT;
            swapBuffers = reinterpret_cast<SwapBuffersFn>(dlsym(h, symSwapBuffers));
            ghostControl = reinterpret_cast<GhostControlFn>(dlsym(h, symGhostControl));

            qInfo() << "EPFrameBuffer: found real EPFramebuffer instance:" << instance
                    << "swapBuffers:" << (swapBuffers ? "yes" : "no")
                    << "ghostControl:" << (ghostControl ? "yes" : "no");
        } else {
            qInfo() << "EPFrameBuffer: real EPFramebuffer not available"
                    << "dlerror:" << dlerror();
        }
    }
};

RealEPFB &realFB() {
    static RealEPFB fb;
    return fb;
}

// Map our WaveformMode enum to the real EPScreenMode
int toScreenMode(EPFrameBuffer::WaveformMode wf, EPFrameBuffer::UpdateMode um) {
    if (um == EPFrameBuffer::FullUpdate) {
        if (wf == EPFrameBuffer::Initialize) return EP_Init;
        return EP_Content;  // GC16-equivalent full refresh
    }
    // Use integer comparison to avoid duplicate case with enum aliases
    int w = static_cast<int>(wf);
    if (w == EPFrameBuffer::Mono)      return EP_Mono;
    if (w == EPFrameBuffer::GC16)      return EP_Content;
    if (w == EPFrameBuffer::GL16)      return EP_FastGrayscale;
    return EP_Content;
}

int toContentType(EPFrameBuffer::WaveformMode wf) {
    int w = static_cast<int>(wf);
    if (w == EPFrameBuffer::Mono) return EP_ContentMono;
    if (w == EPFrameBuffer::GC16) return EP_ContentGrayscale;
    return EP_ContentMixed;
}

} // anonymous namespace

bool EPFrameBuffer::isAvailable()
{
    auto &fb = realFB();
    fb.tryLoad();
    return fb.available;
}

void EPFrameBuffer::sendUpdate(QRect rect, WaveformMode waveform, UpdateMode mode, bool sync)
{
    Q_UNUSED(sync)

    auto &fb = realFB();
    fb.tryLoad();

    if (fb.available && fb.swapBuffers) {
        int screenMode = toScreenMode(waveform, mode);
        int contentType = toContentType(waveform);
        int flags = 0;

        qDebug() << "EPFrameBuffer::sendUpdate: calling swapBuffers"
                 << "rect:" << rect << "contentType:" << contentType
                 << "screenMode:" << screenMode;

        fb.swapBuffers(fb.instance, rect, contentType, screenMode, flags);
        return;
    }

    // Fallback: let QPA handle it
    qDebug() << "EPFrameBuffer::sendUpdate: deferred to QPA"
             << "waveform:" << waveform << "mode:" << mode;
}

void EPFrameBuffer::clearScreen()
{
    auto &fb = realFB();
    fb.tryLoad();

    if (fb.available && fb.ghostControl) {
        qInfo() << "EPFrameBuffer::clearScreen: calling ghostControl";
        fb.ghostControl(fb.instance, 0); // Try mode 0 for clear
    }

    if (auto *window = QGuiApplication::focusWindow()) {
        window->requestUpdate();
    }
}

void EPFrameBuffer::waitForLastUpdate()
{
    // On DRM-based devices, updates are async via the QPA
    qDebug() << "EPFrameBuffer::waitForLastUpdate: no-op (QPA managed)";
}

QImage *EPFrameBuffer::framebuffer()
{
    qDebug() << "EPFrameBuffer::framebuffer: not available (DRM device)";
    return nullptr;
}
