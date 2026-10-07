#pragma once
#include "LayerExtraction.h"

// Mode-specific layer construction. Both write a stereo, pre-saturation mix into `out`.
struct SnareRenderer { static void render(juce::AudioBuffer<float>& out, const LayerMaterial&, const SnairParameters&, double sr, DeterministicRng&); };
struct ClapRenderer  { static void render(juce::AudioBuffer<float>& out, const LayerMaterial&, const SnairParameters&, double sr, DeterministicRng&); };
