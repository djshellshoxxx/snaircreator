#pragma once
#include "Parameters.h"
#include <JuceHeader.h>
#include <vector>

struct FactoryPreset
{
    juce::String name;
    SnairParameters parameters;
};

class FactoryPresets
{
public:
    static const std::vector<FactoryPreset>& all();
};
