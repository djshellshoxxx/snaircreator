#pragma once

#include "SourceAudio.h"

class SourceLoader
{
public:
    static std::shared_ptr<SourceAudio> decode(const juce::File& file,
                                               juce::String& errorMessage);

    static constexpr double maxDurationSeconds = 600.0;
    static constexpr uint64_t maxDecodedBytes = 256ull * 1024ull * 1024ull;
    static constexpr size_t waveformBinCount = 1200;
};
