#include "LayerExtraction.h"
#include <algorithm>
#include <cmath>

namespace snairdsp
{
namespace
{
void filter(std::vector<float>& x, const juce::IIRCoefficients& c)
{
    juce::IIRFilter f;
    f.setCoefficients(c);
    f.processSamples(x.data(), static_cast<int>(x.size()));
}
float safeHz(double sr, float hz) { return std::clamp(hz, 10.0f, static_cast<float>(sr * 0.45)); }
}

void highpass(std::vector<float>& x, double sr, float hz, float q) { filter(x, juce::IIRCoefficients::makeHighPass(sr, safeHz(sr, hz), q)); }
void lowpass(std::vector<float>& x, double sr, float hz, float q) { filter(x, juce::IIRCoefficients::makeLowPass(sr, safeHz(sr, hz), q)); }
void bandpass(std::vector<float>& x, double sr, float hz, float q) { filter(x, juce::IIRCoefficients::makeBandPass(sr, safeHz(sr, hz), q)); }

std::vector<float> noise(int frames, DeterministicRng& rng)
{
    std::vector<float> n(static_cast<size_t>(std::max(0, frames)));
    for (auto& v : n) v = rng.bipolar();
    return n;
}

void addModalBody(float* dst, int frames, double sr, float f0, float tau, float drop, float gain, DeterministicRng& rng)
{
    // Snare shell modes (approximate membrane ratios); upper modes decay faster.
    static constexpr float ratios[] = { 1.0f, 1.59f, 2.14f, 2.65f };
    static constexpr float amps[] = { 1.0f, 0.42f, 0.24f, 0.12f };
    for (int m = 0; m < 4; ++m)
    {
        const float f = f0 * ratios[m] * (1.0f + 0.006f * rng.bipolar());
        if (f > sr * 0.45) continue;
        const float modeTau = tau / (1.0f + 0.9f * static_cast<float>(m));
        double phase = rng.uniform() * 0.2;
        for (int i = 0; i < frames; ++i)
        {
            const float t = static_cast<float>(i / sr);
            const float env = std::exp(-t / modeTau);
            if (env < 1.0e-4f) break;
            const float inst = f * (1.0f + drop * std::exp(-t / 0.012f));
            phase += inst / sr;
            dst[i] += gain * amps[m] * env * std::sin(2.0f * pi * static_cast<float>(phase - std::floor(phase)));
        }
    }
}

int addBursts(float* L, float* R, const LayerMaterial& m, const SnairParameters& p, double sr, int count,
              float spreadMs, float gain, DeterministicRng& rng, std::vector<int>* burstStarts)
{
    const int frames = m.frames;
    const int burstLen = std::max(16, static_cast<int>(sr * (0.006 + 0.010 * p.attack)));
    const float tau = static_cast<float>(burstLen) / 3.2f;
    int start = 0;
    int last = 0;
    for (int b = 0; b < count; ++b)
    {
        if (b > 0)
        {
            // Spacing jitter stays inside the approved 8-35 ms range.
            const float ms = std::clamp(spreadMs * (1.0f + 0.22f * rng.bipolar()), 8.0f, 35.0f);
            start += static_cast<int>(sr * ms / 1000.0);
        }
        if (start >= frames) break;
        last = start;
        if (burstStarts) burstStarts->push_back(start);

        const float centre = 900.0f * (1.0f + 0.9f * p.snap) * (1.0f + 0.18f * rng.bipolar());
        const float amp = (b == count - 1 ? 1.0f : 0.62f + 0.3f * rng.uniform()) * gain;
        const float pan = std::clamp(0.5f + 0.45f * p.width * rng.bipolar(), 0.0f, 1.0f);
        const int len = std::min(burstLen * 2, frames - start);
        const int grainCount = std::max(1, static_cast<int>(m.grains[0].size()));
        const int offset = rng.integer(0, std::max(0, std::min(grainCount - len, static_cast<int>(sr * 0.08))));

        std::vector<float> x(static_cast<size_t>(len));
        auto n = noise(len, rng);
        for (int i = 0; i < len; ++i)
        {
            const int gi = std::min(grainCount - 1, offset + i);
            const float src = m.grains[0].empty() ? 0.0f : 0.5f * (m.grains[0][static_cast<size_t>(gi)] + m.grains[1][static_cast<size_t>(gi)]);
            x[static_cast<size_t>(i)] = m.sourceAmount * 2.2f * src + (0.35f + m.reinforcementAmount) * n[static_cast<size_t>(i)];
        }
        bandpass(x, sr, centre, 0.9f + 0.8f * p.snap);
        for (int i = 0; i < len; ++i)
        {
            const float env = (i < 12 ? i / 12.0f : 1.0f) * std::exp(-static_cast<float>(i) / tau);
            const float y = x[static_cast<size_t>(i)] * env * amp;
            L[start + i] += y * std::sqrt(1.0f - pan);
            R[start + i] += y * std::sqrt(pan);
        }
    }
    return last;
}
}

LayerMaterial LayerExtraction::prepare(const SourceAudio& source, const SourceAnalysis& analysis,
                                       const SnairParameters& p, double sr, int frames, DeterministicRng& rng)
{
    LayerMaterial m;
    m.frames = frames;
    const int n = source.samples.getNumSamples();
    const int chans = std::max(1, source.samples.getNumChannels());

    // Transient choice: strongest by default; the seed may pick another strong candidate,
    // which is how Randomize varies source extraction positions deterministically.
    int64_t chosen = analysis.strongestTransientSample;
    if (!analysis.transientCandidates.empty() && rng.uniform() > 0.6f)
        chosen = analysis.transientCandidates[static_cast<size_t>(rng.integer(0, static_cast<int>(analysis.transientCandidates.size()) - 1))];
    const int pre = static_cast<int>(source.sampleRate * 0.001);
    const int startSample = std::clamp(static_cast<int>(chosen) - pre, 0, std::max(0, n - 1));
    m.attackSourceSample = startSample;

    // Resample from the source rate to the output rate with pitch shift (cubic Hermite).
    const double step = (source.sampleRate / sr) * std::pow(2.0, p.pitchSt / 12.0);
    for (int ch = 0; ch < 2; ++ch)
    {
        auto& dst = m.region[ch];
        dst.assign(static_cast<size_t>(frames), 0.0f);
        const float* s = source.samples.getReadPointer(std::min(ch, chans - 1));
        auto at = [&](int i) { return (i >= 0 && i < n) ? s[i] : 0.0f; };
        for (int i = 0; i < frames; ++i)
        {
            const double pos = startSample + i * step;
            const int k = static_cast<int>(pos);
            if (k >= n) break;
            const float f = static_cast<float>(pos - k);
            const float y0 = at(k - 1), y1 = at(k), y2 = at(k + 1), y3 = at(k + 2);
            const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            dst[static_cast<size_t>(i)] = ((c3 * f + c2) * f + c1) * f + y1;
            m.validFrames = i + 1;
        }
    }

    // Scrambled grains: Hann-windowed 6 ms grains read from deterministic offsets around the transient
    // region. This decorrelates tonal sources into usable texture while keeping their spectrum.
    const int available = std::max(1, m.validFrames);
    const int span = std::max(1, std::min(available, static_cast<int>(sr * 0.35)));
    const int grain = std::max(8, static_cast<int>(sr * 0.006));
    const int hop = grain / 2;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto& g = m.grains[ch];
        g.assign(static_cast<size_t>(frames), 0.0f);
        for (int start = 0; start < frames; start += hop)
        {
            const int off = rng.integer(0, std::max(0, span - grain));
            for (int i = 0; i < grain && start + i < frames; ++i)
            {
                const int si = off + i;
                if (si >= available) break;
                const float w = 0.5f - 0.5f * std::cos(2.0f * snairdsp::pi * static_cast<float>(i) / static_cast<float>(grain - 1));
                g[static_cast<size_t>(start + i)] += m.region[ch][static_cast<size_t>(si)] * w;
            }
        }
    }

    // Normalise source material so quiet sources still contribute (peak-based, bounded gain).
    float peak = 1.0e-6f;
    for (int ch = 0; ch < 2; ++ch)
        for (auto v : m.region[ch]) peak = std::max(peak, std::abs(v));
    const float gain = std::min(1.0f / peak, 40.0f);
    float gpeak = 1.0e-6f;
    for (int ch = 0; ch < 2; ++ch)
    {
        for (auto& v : m.region[ch]) v *= gain;
        for (auto v : m.grains[ch]) gpeak = std::max(gpeak, std::abs(v));
    }
    for (int ch = 0; ch < 2; ++ch)
        for (auto& v : m.grains[ch]) v *= std::min(1.0f / gpeak, 40.0f);

    m.bodyFrequencyHz = std::clamp(0.6f * p.bodyFreqHz + 0.4f * std::clamp(analysis.dominantBodyHz, 110.0f, 320.0f), 70.0f, 450.0f);
    m.requiresFallback = analysis.crest < 1.35f || analysis.transientCandidates.empty() || m.validFrames < static_cast<int>(sr * 0.004);
    m.sourceAmount = 0.15f + 0.85f * p.sourceCharacter;
    m.reinforcementAmount = std::clamp(1.0f - 0.75f * p.sourceCharacter + (m.requiresFallback ? 0.3f : 0.0f), 0.1f, 1.0f);
    const float tail = std::clamp(p.tailMs / 1000.0f, 0.02f, 2.0f);
    m.tailTau = std::clamp(tail * (0.18f + 0.32f * p.size), 0.006f, 1.0f);
    m.bodyTau = 0.03f + 0.12f * p.body + 0.16f * p.size;
    m.noisiness = std::clamp(analysis.noisyEstimate, 0.0f, 1.0f);
    return m;
}
