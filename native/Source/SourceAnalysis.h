#pragma once
#include <JuceHeader.h>
#include <cstdint>
#include <vector>

struct SourceAnalysis
{
    double durationSeconds = 0.0;
    double sampleRate = 0.0;
    int channels = 0;
    int64_t fileSizeBytes = 0;

    float peak = 0.0f;
    float rms = 0.0f;
    float crest = 0.0f;
    float dcOffset = 0.0f;
    float zeroCrossingRate = 0.0f;

    float spectralCentroidHz = 0.0f;
    float spectralRolloffHz = 0.0f;
    float spectralFlatness = 0.0f;
    float lowEnergyRatio = 0.0f;
    float midEnergyRatio = 0.0f;
    float highEnergyRatio = 0.0f;
    float dominantBodyHz = 190.0f;
    float noisyEstimate = 0.5f;

    int64_t strongestTransientSample = 0;
    std::vector<int64_t> transientCandidates;
    float transientDensityPerSecond = 0.0f;

    int64_t attackWindowStart = 0;
    int64_t attackWindowEnd = 0;
    int64_t tailWindowStart = 0;
    int64_t tailWindowEnd = 0;

    uint64_t fingerprint = 0;
};
