#pragma once
#include "Parameters.h"
#include "RenderedHit.h"
#include "SourceAnalysis.h"
#include "SourceAudio.h"
#include <atomic>

// Offline render orchestration and parameter variation. Never called on the audio thread.
class SnairEngine
{
public:
    static constexpr double maxHitSeconds = 3.0;

    static std::shared_ptr<RenderedHit> render(const SourceAudio&, const SourceAnalysis&, SnairParameters,
                                               uint64_t generationId, juce::String& error,
                                               double outputSampleRate = 0.0,
                                               const std::atomic<bool>* cancel = nullptr);
    static int framesFor(const SnairParameters&, double sampleRate);
    static SnairParameters randomized(SnairParameters p, uint32_t variationSeed);
    static SnairParameters mutated(SnairParameters p, uint32_t variationSeed);
};
