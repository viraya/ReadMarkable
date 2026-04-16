#include "WaveformStrategy.h"

WaveformStrategy::RefreshParams WaveformStrategy::paramsFor(DisplayOperation op)
{
    switch (op) {
    case DisplayOperation::PageTurn:

        return {EPFrameBuffer::Mono, EPFrameBuffer::PartialUpdate, false};

    case DisplayOperation::FullClear:

        return {EPFrameBuffer::GC16, EPFrameBuffer::FullUpdate, true};

    case DisplayOperation::Initialize:

        return {EPFrameBuffer::Initialize, EPFrameBuffer::FullUpdate, true};

    case DisplayOperation::UiOverlay:

        return {EPFrameBuffer::Mono, EPFrameBuffer::PartialUpdate, false};

    case DisplayOperation::PenStroke:

        return {EPFrameBuffer::DU, EPFrameBuffer::PartialUpdate, false};

    case DisplayOperation::ImageRender:

        return {EPFrameBuffer::GC16, EPFrameBuffer::PartialUpdate, true};
    }

    return {EPFrameBuffer::Mono, EPFrameBuffer::PartialUpdate, false};
}
