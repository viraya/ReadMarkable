#pragma once

#include <epframebuffer.h>

enum class DisplayOperation {
    PageTurn,
    FullClear,
    Initialize,
    UiOverlay,
    PenStroke,
    ImageRender,
};

class WaveformStrategy {
public:
    struct RefreshParams {
        EPFrameBuffer::WaveformMode waveform;
        EPFrameBuffer::UpdateMode updateMode;
        bool sync;
    };

    static RefreshParams paramsFor(DisplayOperation op);
};
