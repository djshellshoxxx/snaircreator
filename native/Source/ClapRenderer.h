#pragma once
#include "DeterministicRng.h"
#include "LayerExtraction.h"
#include "Parameters.h"
#include "SourceAudio.h"
#include <JuceHeader.h>

class ClapRenderer
{
public:
    static void render(juce::AudioBuffer<float>&,
                       const SourceAudio&,
                       const SnairParameters&,
                       const LayerMaterial&,
                       double sampleRate,
                       DeterministicRng&);
};
