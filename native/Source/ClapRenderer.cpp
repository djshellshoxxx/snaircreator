#include "Renderers.h"
#include <cmath>

using namespace snairdsp;

namespace
{
// Schroeder allpass used for optional tail diffusion/stereo decorrelation.
void allpass(std::vector<float>& x, int delay, float g)
{
    std::vector<float> buf(static_cast<size_t>(delay), 0.0f);
    size_t w = 0;
    for (auto& v : x)
    {
        const float d = buf[w];
        const float in = v + g * d;
        v = d - g * in;
        buf[w] = in;
        w = (w + 1) % buf.size();
    }
}
}

// clap = burst1 + ... + burstN + texture tail (+ cross-blend snare body)
void ClapRenderer::render(juce::AudioBuffer<float>& out, const LayerMaterial& m, const SnairParameters& p,
                          double sr, DeterministicRng& rng)
{
    const int N = out.getNumSamples();
    float* L = out.getWritePointer(0);
    float* R = out.getWritePointer(1);
    const int last = addBursts(L, R, m, p, sr, p.clapCount, p.clapSpreadMs, 0.55f + 0.6f * p.punch, rng);

    const float tailLevel = (0.25f + 0.75f * p.texture) * (0.35f + 0.65f * p.noise);
    const float centre = 1100.0f + 900.0f * p.snap;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto tail = m.grains[ch];
        auto n = noise(N, rng);
        for (int i = 0; i < N; ++i)
            tail[static_cast<size_t>(i)] = tail[static_cast<size_t>(i)] * m.sourceAmount * 1.4f
                                         + n[static_cast<size_t>(i)] * (1.1f - 0.7f * m.sourceAmount);
        bandpass(tail, sr, centre, 0.7f);
        highpass(tail, sr, 450.0f);
        if (p.size > 0.4f)
        {
            const float g = 0.35f + 0.3f * p.size;
            allpass(tail, static_cast<int>(sr * (ch ? 0.0047 : 0.0041)), g);
            allpass(tail, static_cast<int>(sr * (ch ? 0.0113 : 0.0097)), g);
        }
        // Body: low-mid source weight under the clap.
        auto weight = m.region[ch];
        bandpass(weight, sr, 650.0f, 0.8f);

        float* y = out.getWritePointer(ch);
        for (int i = 0; i < N; ++i)
        {
            const float rel = static_cast<float>((i - last) / sr);
            const float env = i < last ? 0.25f * std::exp(-static_cast<float>(last - i) / static_cast<float>(sr * 0.01))
                                       : std::exp(-rel / m.tailTau) * (rel < 0.002f ? 0.6f + 200.0f * rel : 1.0f);
            const float wEnv = std::exp(-static_cast<float>(i / sr) / (0.02f + 0.05f * p.body));
            y[i] += tail[static_cast<size_t>(i)] * env * tailLevel * 0.55f
                  + weight[static_cast<size_t>(i)] * wEnv * m.sourceAmount * 0.35f * p.body;
        }
    }

    if (p.crossBlend > 0.001f)
    {
        std::vector<float> body(static_cast<size_t>(N), 0.0f);
        addModalBody(body.data(), N, sr, m.bodyFrequencyHz, m.bodyTau, 0.3f, 0.7f * p.crossBlend, rng);
        for (int i = 0; i < N; ++i) { L[i] += body[static_cast<size_t>(i)]; R[i] += body[static_cast<size_t>(i)]; }
    }
}
