#pragma once
#include "SourceAudio.h"
#include "SourceAnalysis.h"
#include <memory>

class SourceAnalyzer
{
public:
    static bool analyze(const SourceAudio& source, SourceAnalysis& result, juce::String& error);
};
