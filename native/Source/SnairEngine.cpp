#include "SnairEngine.h"
#include "Renderers.h"
#include <algorithm>
#include <cmath>

namespace
{
void applyWidth(juce::AudioBuffer<float>& b, float width)
{
    // width 0 = mono, 0.5 = natural, 1 = wide
    const float sideGain = width * 2.0f;
    auto* L = b.getWritePointer(0);
    auto* R = b.getWritePointer(1);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float mid = 0.5f * (L[i] + R[i]);
        const float side = 0.5f * (L[i] - R[i]) * sideGain;
        L[i] = mid + side;
        R[i] = mid - side;
    }
}

void applySaturation(juce::AudioBuffer<float>& b, const SnairParameters& p, double sr)
{
    const float g = juce::Decibels::decibelsToGain(p.driveDb) * (1.0f + 2.5f * p.dirt) * (1.0f + 0.4f * p.punch);
    const float norm = 1.0f / std::sqrt(g);
    const float asym = 0.25f * p.dirt;
    const float crush = p.dirt > 0.6f ? std::pow(2.0f, 16.0f - 22.0f * (p.dirt - 0.6f)) : 0.0f; // source crunch
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* y = b.getWritePointer(ch);
        float dcX = 0.0f, dcY = 0.0f;
        const float r = 1.0f - static_cast<float>(2.0 * juce::MathConstants<double>::pi * 20.0 / sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float x = y[i] * g;
            x = std::tanh(x + asym * x * x) * norm;
            if (crush > 0.0f) x = std::round(x * crush) / crush;
            const float out = x - dcX + r * dcY; // DC blocker removes asymmetry offset
            dcX = x; dcY = out;
            y[i] = out;
        }
    }
}

// Gated room: a short dense reverb whose tail is hard-gated, the classic 80s snare effect.
void applyGatedRoom(juce::AudioBuffer<float>& b, const SnairParameters& p, double sr)
{
    if (p.room < 0.001f) return;
    juce::AudioBuffer<float> wet(b);
    juce::Reverb reverb;
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.55f + 0.4f * p.size;
    rp.damping = 0.35f - 0.25f * p.snap;
    rp.wetLevel = 1.0f; rp.dryLevel = 0.0f; rp.width = 0.6f + 0.4f * p.width;
    reverb.setParameters(rp);
    reverb.setSampleRate(sr);
    reverb.processStereo(wet.getWritePointer(0), wet.getWritePointer(1), wet.getNumSamples());
    const int open = static_cast<int>(sr * (0.07 + 0.22 * p.room + 0.08 * p.size));
    const int release = std::max(1, static_cast<int>(sr * 0.012));
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* w = wet.getReadPointer(ch);
        auto* y = b.getWritePointer(ch);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float gate = i < open ? 1.0f : (i < open + release ? 1.0f - static_cast<float>(i - open) / release : 0.0f);
            y[i] += w[i] * gate * p.room * 1.1f;
        }
    }
}

void applyTone(juce::AudioBuffer<float>& b, float tone, double sr)
{
    if (std::abs(tone) < 1.0e-4f) return;
    const float a = 1.0f - std::exp(-2.0f * snairdsp::pi * 900.0f / static_cast<float>(sr));
    const float lowGain = 1.0f - 0.45f * tone, highGain = 1.0f + (tone > 0 ? 0.9f : 0.75f) * tone;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* y = b.getWritePointer(ch);
        float lp = 0.0f;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            lp += a * (y[i] - lp);
            y[i] = lp * lowGain + (y[i] - lp) * highGain;
        }
    }
}

void applySafety(RenderedHit& hit, bool normalize)
{
    auto& b = hit.samples;
    const int n = b.getNumSamples();
    const int fadeIn = std::min(n / 4, 16);
    const int fadeOut = std::min(n / 4, static_cast<int>(hit.sampleRate * 0.01));
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        auto* y = b.getWritePointer(ch);
        for (int i = 0; i < n; ++i)
        {
            float v = y[i];
            if (!std::isfinite(v) || std::abs(v) < 1.0e-20f) v = 0.0f; // NaN/Inf/denormal guard
            if (i < fadeIn) v *= static_cast<float>(i) / static_cast<float>(fadeIn);
            if (i >= n - fadeOut) v *= static_cast<float>(n - 1 - i) / static_cast<float>(fadeOut);
            y[i] = v;
        }
    }
    float peak = b.getMagnitude(0, n);
    if (peak > 1.0e-7f && (normalize || peak > 0.98f))
        b.applyGain(0.98f / peak);
    hit.peak = b.getMagnitude(0, n);

    constexpr int bins = 400;
    hit.peaks.assign(bins, 0.0f);
    for (int i = 0; i < n; ++i)
    {
        const size_t bin = std::min<size_t>(bins - 1, static_cast<size_t>(static_cast<int64_t>(i) * bins / n));
        hit.peaks[bin] = std::max({ hit.peaks[bin], std::abs(b.getSample(0, i)), std::abs(b.getSample(1, i)) });
    }
}
}

int SnairEngine::framesFor(const SnairParameters& p, double sr)
{
    const float tail = std::clamp(p.tailMs / 1000.0f, 0.02f, 2.0f);
    const float tau = std::clamp(tail * (0.18f + 0.32f * p.size), 0.006f, 1.0f);
    const float bodyTau = 0.03f + 0.12f * p.body + 0.16f * p.size;
    const float burstSpan = p.mode == SnairMode::clap ? (p.clapCount - 1) * p.clapSpreadMs * 1.25f / 1000.0f : 0.0f;
    const float seconds = std::max(5.5f * tau, 5.0f * bodyTau) + burstSpan + 0.03f + 0.12f * p.room;
    return static_cast<int>(std::ceil(std::clamp(static_cast<double>(seconds), 0.06, maxHitSeconds) * sr));
}

std::shared_ptr<RenderedHit> SnairEngine::render(const SourceAudio& source, const SourceAnalysis& analysis,
                                                 SnairParameters p, uint64_t generationId, juce::String& error,
                                                 double outputSampleRate, const std::atomic<bool>* cancel)
{
    juce::ScopedNoDenormals noDenormals;
    error.clear();
    p.sanitize();
    if (source.samples.getNumSamples() < 2 || source.sampleRate <= 0.0 || source.samples.getNumChannels() < 1)
    {
        error = "No valid source is loaded.";
        return {};
    }
    const double sr = (std::isfinite(outputSampleRate) && outputSampleRate >= 8000.0 && outputSampleRate <= 384000.0)
                    ? outputSampleRate : source.sampleRate;
    const int frames = framesFor(p, sr);

    auto hit = std::make_shared<RenderedHit>();
    hit->samples.setSize(2, frames);
    hit->samples.clear();
    hit->sampleRate = sr;
    hit->sourceFingerprint = analysis.fingerprint;
    hit->paramHash = p.renderHash();
    hit->seed = p.seed;
    hit->generationId = generationId;

    DeterministicRng rng((static_cast<uint64_t>(p.seed) << 1) ^ analysis.fingerprint ^ (p.mode == SnairMode::clap ? 0xC1A9ull : 0x5A4Eull));
    const auto material = LayerExtraction::prepare(source, analysis, p, sr, frames, rng);
    hit->usedFallback = material.requiresFallback;
    hit->attackSourceSample = material.attackSourceSample;
    if (cancel && cancel->load()) { error = "Render superseded."; return {}; }

    if (p.mode == SnairMode::snare) SnareRenderer::render(hit->samples, material, p, sr, rng);
    else                            ClapRenderer::render(hit->samples, material, p, sr, rng);
    if (cancel && cancel->load()) { error = "Render superseded."; return {}; }

    applyWidth(hit->samples, p.width);
    applySaturation(hit->samples, p, sr);
    applyGatedRoom(hit->samples, p, sr);
    applyTone(hit->samples, p.tone, sr);
    applySafety(*hit, p.normalizeRender);

    if (hit->peak <= 1.0e-6f)
    {
        error = "The render produced no audible output. Try raising Texture or Source Character.";
        return {};
    }
    return hit;
}

SnairParameters SnairEngine::randomized(SnairParameters p, uint32_t seed)
{
    DeterministicRng rng(seed);
    auto r = [&](float lo, float hi) { return lo + (hi - lo) * rng.uniform(); };
    p.seed = 1u + static_cast<uint32_t>(rng.next() % maxGeneratedSeed);
    p.sourceCharacter = r(0.2f, 0.9f);
    p.punch = r(0.25f, 1.0f); p.snap = r(0.2f, 1.0f); p.body = r(0.15f, 0.95f);
    p.texture = r(0.15f, 1.0f); p.dirt = r(0.0f, 0.7f); p.size = r(0.15f, 0.9f);
    p.bodyFreqHz = r(110.0f, 320.0f);
    p.pitchSt = r(-7.0f, 7.0f); p.tone = r(-0.6f, 0.7f); p.driveDb = r(0.0f, 12.0f);
    p.width = r(0.2f, 0.9f); p.tailMs = r(90.0f, 700.0f); p.attack = r(0.15f, 0.9f); p.noise = r(0.2f, 0.95f);
    if (p.mode == SnairMode::clap) { p.clapCount = rng.integer(3, 6); p.clapSpreadMs = r(9.0f, 30.0f); }
    p.crossBlend = rng.uniform() > 0.7f ? r(0.0f, 0.35f) : 0.0f;
    p.sanitize();
    return p;
}

SnairParameters SnairEngine::mutated(SnairParameters p, uint32_t seed)
{
    DeterministicRng rng(seed);
    // Each continuous value moves at most ~15% of its useful normalized range.
    auto mvs = [&](float v, float span, float lo, float hi) { return std::clamp(v + rng.bipolar() * 0.15f * span, lo, hi); };
    auto mv = [&](float v, float lo, float hi) { return mvs(v, hi - lo, lo, hi); };
    p.seed = 1u + static_cast<uint32_t>(rng.next() % maxGeneratedSeed);
    p.sourceCharacter = mv(p.sourceCharacter, 0, 1); p.punch = mv(p.punch, 0, 1); p.snap = mv(p.snap, 0, 1);
    p.body = mv(p.body, 0, 1); p.texture = mv(p.texture, 0, 1); p.dirt = mv(p.dirt, 0, 1); p.size = mv(p.size, 0, 1);
    p.bodyFreqHz = mvs(p.bodyFreqHz, 210, 70, 450); p.pitchSt = mvs(p.pitchSt, 24, -24, 24); p.tone = mv(p.tone, -1, 1);
    p.driveDb = mvs(p.driveDb, 18, 0, 24); p.width = mv(p.width, 0, 1); p.tailMs = mvs(p.tailMs, 780, 20, 2000);
    p.attack = mv(p.attack, 0, 1); p.noise = mv(p.noise, 0, 1);
    if (p.mode == SnairMode::clap)
    {
        if (rng.uniform() > 0.65f) p.clapCount += rng.uniform() > 0.5f ? 1 : -1;
        p.clapSpreadMs = mv(p.clapSpreadMs, 8, 35);
    }
    p.sanitize();
    return p;
}
