#pragma once
#include "RenderedHit.h"
#include <JuceHeader.h>

struct WavExportOptions
{
    int bitDepth = 24;
    double sampleRate = 0.0;
    int channels = 2;
    bool normalize = false;
    bool includeOutputTrim = true;
    float outputTrimDb = 0.0f;
};

class WavExporter
{
public:
    static bool write(const juce::File&, const RenderedHit&, const WavExportOptions&, juce::String& error);
};
