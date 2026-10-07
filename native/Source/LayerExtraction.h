#pragma once
#include "DeterministicRng.h"
#include "Parameters.h"
#include "SourceAnalysis.h"
#include "SourceAudio.h"
#include <vector>

// Source-derived material prepared for one render, already at the output rate and pitch.
struct LayerMaterial
{
    int frames = 0;                       // output frames
    int validFrames = 0;                  // frames backed by real source audio (rest is silence)
    int64_t attackSourceSample = 0;
    std::vector<float> region[2];         // pitched source from the selected transient, L/R
    std::vector<float> grains[2];         // scrambled source grains (texture/decorrelation), L/R
    float bodyFrequencyHz = 190.0f;
    float sourceAmount = 0.5f;            // Source Character: how much source passes through
    float reinforcementAmount = 0.5f;     // procedural support, falls as Source Character rises
    float tailTau = 0.1f;                 // seconds
    float bodyTau = 0.1f;                 // seconds
    float noisiness = 0.5f;
    bool requiresFallback = false;
};

class LayerExtraction
{
public:
    static LayerMaterial prepare(const SourceAudio&, const SourceAnalysis&, const SnairParameters&,
                                 double outputSampleRate, int frames, DeterministicRng&);
};

namespace snairdsp
{
constexpr float pi = 3.14159265358979323846f;
void highpass(std::vector<float>&, double sr, float hz, float q = 0.707f);
void lowpass(std::vector<float>&, double sr, float hz, float q = 0.707f);
void bandpass(std::vector<float>&, double sr, float hz, float q);
std::vector<float> noise(int frames, DeterministicRng&);
// Damped modal body with a short pitch drop; adds into dst.
void addModalBody(float* dst, int frames, double sr, float f0, float tau, float drop, float gain, DeterministicRng&);
// Clap-style micro bursts; adds into L/R. Returns the time (in frames) of the last burst.
int addBursts(float* L, float* R, const LayerMaterial&, const SnairParameters&, double sr, int count,
              float spreadMs, float gain, DeterministicRng&, std::vector<int>* burstStarts = nullptr);
}
