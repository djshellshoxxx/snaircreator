#pragma once
#include "Parameters.h"
#include <JuceHeader.h>

class PresetManager
{
public:
    static bool save(const juce::File&, const SnairParameters&, juce::String& error);
    static bool load(const juce::File&, SnairParameters&, juce::String& error);
};
