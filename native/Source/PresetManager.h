#pragma once
#include "Parameters.h"
#include <JuceHeader.h>

// JSON preset files (*.snairpreset). Presets store parameters + seed and an external source reference
// (path/fingerprint) only; source audio is never embedded in a preset.
class PresetManager
{
public:
    static constexpr const char* extension = ".snairpreset";
    static juce::File userFolder();
    static bool save(const juce::File&, const SnairParameters&, const juce::String& sourcePath, uint64_t sourceFingerprint, juce::String& error);
    // On success fills p; `warning` lists invalid fields that fell back to defaults; `sourcePath` returns the reference.
    static bool load(const juce::File&, SnairParameters& p, juce::String& error, juce::String& warning, juce::String& sourcePath);
};
