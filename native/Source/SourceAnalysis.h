#pragma once
#include <JuceHeader.h>
#include <cstdint>
#include <vector>

struct SourceAnalysis
{
    double durationSeconds = 0.0;
    double sampleRate = 0.0;
    int channels = 0;
    float peak = 0.0f;
    float rms = 0.0f;
    float crest = 0.0f;
    float zeroCrossingRate = 0.0f;
    float spectralCentroidHz = 0.0f;
    float lowEnergyRatio = 0.0f;
    float midEnergyRatio = 0.0f;
    float highEnergyRatio = 0.0f;
    float dominantBodyHz = 190.0f;
    float noisyEstimate = 0.5f;
    int64_t strongestTransientSample = 0;
    std::vector<int64_t> transientCandidates;
    uint64_t fingerprint = 0;
};
