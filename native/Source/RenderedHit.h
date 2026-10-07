#pragma once
#include <JuceHeader.h>
#include <cstdint>
#include <memory>

struct RenderedHit
{
    juce::AudioBuffer<float> samples;  // always stereo, finite, pre-output-trim
    double sampleRate = 48000.0;
    float peak = 0.0f;
    uint64_t sourceFingerprint = 0;
    uint64_t paramHash = 0;
    uint32_t seed = 0;
    uint64_t generationId = 0;
    int64_t attackSourceSample = 0;    // source position used for the attack layer
    bool usedFallback = false;
    std::vector<float> peaks;          // display peaks for the generated-hit waveform
};
using RenderedHitPtr = std::shared_ptr<const RenderedHit>;
