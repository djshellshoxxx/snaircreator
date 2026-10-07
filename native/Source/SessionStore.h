#pragma once
#include "Parameters.h"
#include "RenderedHit.h"
#include <JuceHeader.h>

struct RestoredSession
{
    SnairParameters parameters;
    RenderedHitPtr hit;
    juce::File sourceFile;
};

class SessionStore
{
public:
    static juce::File defaultDirectory();
    static bool save(const juce::File& directory,
                     const SnairParameters& parameters,
                     const RenderedHitPtr& hit,
                     const juce::File& sourceFile,
                     juce::String& error);
    static bool load(const juce::File& directory,
                     RestoredSession& session,
                     juce::String& error);
};
