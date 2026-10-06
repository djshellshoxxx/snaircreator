#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

struct SourceAudio
{
    juce::File sourceFile;
    juce::AudioBuffer<float> samples;
    std::vector<float> waveformPeaks;
    double sampleRate = 0.0;
    int channelCount = 0;
    int64_t frameCount = 0;
    int sanitizedSampleCount = 0;

    double durationSeconds() const noexcept
    {
        return sampleRate > 0.0 ? static_cast<double>(frameCount) / sampleRate : 0.0;
    }
};

using SourceAudioPtr = std::shared_ptr<const SourceAudio>;
