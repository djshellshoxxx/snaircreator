#pragma once
#include "Parameters.h"
#include "RenderedHit.h"
#include "SourceAnalysis.h"
#include "SourceAudio.h"

class SnairEngine
{
public:
    static std::shared_ptr<RenderedHit> render(const SourceAudio&, const SourceAnalysis&, SnairParameters, uint64_t generationId, juce::String& error);
    static SnairParameters randomized(SnairParameters p, uint32_t variationSeed);
    static SnairParameters mutated(SnairParameters p, uint32_t variationSeed);
};
