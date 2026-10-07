#include "Renderers.h"
#include <cmath>

using namespace snairdsp;

// snare = attack/crack + body + wire/texture + optional reinforcement (+ cross-blend bursts)
void SnareRenderer::render(juce::AudioBuffer<float>& out, const LayerMaterial& m, const SnairParameters& p,
                           double sr, DeterministicRng& rng)
{
    const int N = out.getNumSamples();
    // Higher Source Character keeps more of the source transient (longer window, gentler forcing).
    const int attackLen = std::max(16, static_cast<int>(sr * (0.005 + 0.035 * p.attack) * (0.7f + 0.6f * p.sourceCharacter)));
    const float attackCurve = 1.0f + 2.5f * p.punch * (1.0f - 0.5f * p.sourceCharacter);
    const float crackHz = juce::jmap(p.sourceCharacter, 900.0f, 120.0f) + 700.0f * p.snap;

    // Synth layers shared by both channels.
    std::vector<float> body(static_cast<size_t>(N), 0.0f);
    addModalBody(body.data(), N, sr, m.bodyFrequencyHz, m.bodyTau, 0.15f + 0.45f * p.punch,
                 m.reinforcementAmount * (0.25f + 0.75f * p.body), rng);
    // Source-informed modal resonance: the source attack excites a resonator at the body frequency.
    std::vector<float> excite(static_cast<size_t>(N), 0.0f);
    for (int i = 0; i < std::min(N, attackLen * 2); ++i)
        excite[static_cast<size_t>(i)] = 0.5f * (m.region[0][static_cast<size_t>(i)] + m.region[1][static_cast<size_t>(i)]);
    auto ring = excite;
    bandpass(ring, sr, m.bodyFrequencyHz, 10.0f);
    bandpass(ring, sr, m.bodyFrequencyHz, 4.0f);

    const float wireHp = 1500.0f * (1.0f + 0.7f * p.snap);
    const float wireLp = std::min(12000.0f, 6500.0f + 5500.0f * p.snap);
    const float wireLevel = (0.12f + 0.88f * p.texture) * (0.3f + 0.7f * p.noise);
    const float clickLevel = m.reinforcementAmount * (0.25f + 0.75f * p.snap);

    for (int ch = 0; ch < 2; ++ch)
    {
        float* y = out.getWritePointer(ch);
        auto crack = m.region[ch];
        highpass(crack, sr, crackHz);
        auto srcBody = m.region[ch];
        bandpass(srcBody, sr, m.bodyFrequencyHz, 2.5f);
        auto wire = m.grains[ch];
        highpass(wire, sr, wireHp);
        lowpass(wire, sr, wireLp);
        auto white = noise(N, rng);
        highpass(white, sr, wireHp);
        lowpass(white, sr, wireLp);
        auto click = noise(N, rng);
        highpass(click, sr, 2500.0f + 2500.0f * p.snap);

        for (int i = 0; i < N; ++i)
        {
            const size_t k = static_cast<size_t>(i);
            const float t = static_cast<float>(i / sr);
            const float aEnv = i < attackLen ? std::pow(1.0f - static_cast<float>(i) / attackLen, attackCurve) : 0.0f;
            const float bEnv = std::exp(-t / m.bodyTau);
            const float wEnv = (i < 48 ? i / 48.0f : 1.0f) * std::exp(-t / m.tailTau);
            const float cEnv = std::exp(-t / 0.0025f);

            const float attackL = crack[k] * aEnv * m.sourceAmount * (0.5f + 1.0f * p.punch) + click[k] * cEnv * clickLevel * 0.35f;
            const float bodyL = (body[k] + (srcBody[k] * bEnv * 0.7f + ring[k] * 6.0f) * m.sourceAmount * (0.3f + 0.7f * p.body))
                              * (0.6f + 0.4f * p.punch);
            const float srcWire = m.sourceAmount * (0.55f + 0.45f * m.noisiness);
            const float wireL = (wire[k] * srcWire * 1.6f + white[k] * (1.15f - 0.75f * m.sourceAmount)) * wEnv * wireLevel;
            y[i] += attackL + bodyL + wireL;
        }
    }

    if (p.crossBlend > 0.001f)
        addBursts(out.getWritePointer(0), out.getWritePointer(1), m, p, sr, 3, 12.0f, 0.6f * p.crossBlend, rng);
}
