#pragma once
#include <JuceHeader.h>
#include <cstdint>
#include <memory>

struct RenderedHit
{
    juce::AudioBuffer<float> samples;
    double sampleRate = 48000.0;
    float peak = 0.0f;
    uint64_t sourceFingerprint = 0;
    uint32_t seed = 0;
    uint64_t generationId = 0;
    bool usedFallback = false;
};
using RenderedHitPtr = std::shared_ptr<const RenderedHit>;
