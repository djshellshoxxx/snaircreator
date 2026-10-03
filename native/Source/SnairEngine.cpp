#include "SnairEngine.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace snair {
namespace {
static float clampf(float v, float lo, float hi) { return juce::jlimit(lo, hi, std::isfinite(v) ? v : lo); }
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 0x6d2b79f5u) {}
    float next() {
        s += 0x6d2b79f5u;
        uint32_t t = s;
        t = (t ^ (t >> 15)) * (t | 1u);
        t ^= t + (t ^ (t >> 7)) * (t | 61u);
        return static_cast<float>((t ^ (t >> 14)) * (1.0 / 4294967296.0));
    }
};
static float sourceAt(const std::vector<float>& x, double index, double pitchRatio) {
    if (x.empty()) return 0.0f;
    const double pos = std::fmod(std::abs(index * pitchRatio), static_cast<double>(x.size()));
    const auto i0 = static_cast<size_t>(pos);
    const auto i1 = (i0 + 1) % x.size();
    const float f = static_cast<float>(pos - static_cast<double>(i0));
    return x[i0] * (1.0f - f) + x[i1] * f;
}
}

Params SnairEngine::sanitize(Params p) {
    p.mode = juce::jlimit(0, 2, p.mode);
    p.seed = juce::jlimit(0, 2147483647, p.seed);
    p.character = clampf(p.character, 0, 1); p.body = clampf(p.body, 0, 1);
    p.bodyFreqHz = clampf(p.bodyFreqHz, 70, 450); p.crack = clampf(p.crack, 0, 1);
    p.noise = clampf(p.noise, 0, 1); p.tailMs = clampf(p.tailMs, 20, 1200);
    p.clapCount = juce::jlimit(2, 6, p.clapCount); p.clapSpreadMs = clampf(p.clapSpreadMs, 8, 35);
    p.width = clampf(p.width, 0, 1); p.driveDb = clampf(p.driveDb, 0, 18);
    p.tone = clampf(p.tone, -1, 1); p.pitchSt = clampf(p.pitchSt, -24, 24);
    p.trimDb = clampf(p.trimDb, -24, 12); p.outputMs = clampf(p.outputMs, 40, 2000);
    return p;
}

Analysis SnairEngine::analyse(const std::vector<float>& samples, double sampleRate) {
    Analysis a;
    if (samples.empty() || sampleRate <= 0) return a;
    a.frames = static_cast<int>(samples.size()); a.duration = samples.size() / sampleRate;
    double ss = 0.0; int zc = 0; float strongest = -1.0f, prev = std::isfinite(samples[0]) ? samples[0] : 0.0f;
    for (size_t i = 0; i < samples.size(); ++i) {
        const float x = std::isfinite(samples[i]) ? samples[i] : 0.0f;
        a.peak = std::max(a.peak, std::abs(x)); ss += static_cast<double>(x) * x;
        if (i > 0) {
            if ((x >= 0) != (prev >= 0)) ++zc;
            const float d = std::abs(x - prev);
            if (d > strongest) { strongest = d; a.onsetIndex = static_cast<int>(i); }
        }
        prev = x;
    }
    a.rms = static_cast<float>(std::sqrt(ss / samples.size()));
    a.zeroCrossingRate = static_cast<float>(zc / std::max(1.0, static_cast<double>(samples.size() - 1)));
    const float rough = 90.0f + std::min(1.0f, a.zeroCrossingRate * 18.0f) * 260.0f;
    a.bodyFreqHint = juce::jlimit(110.0f, 320.0f, rough);
    return a;
}

std::vector<float> SnairEngine::render(const std::vector<float>& input, double sampleRate, Params params) {
    params = sanitize(params);
    const double sr = sampleRate > 1000 ? sampleRate : 48000.0;
    const std::vector<float> fallback { 0.0f };
    const auto& src = input.empty() ? fallback : input;
    const auto a = analyse(src, sr);
    Rng rng(static_cast<uint32_t>(params.seed) ^ static_cast<uint32_t>(a.rms * 1.0e9f) ^ static_cast<uint32_t>(a.onsetIndex));
    const int frames = std::max(1, juce::roundToInt(sr * params.outputMs / 1000.0));
    std::vector<float> out(static_cast<size_t>(frames), 0.0f);
    const double pitch = std::pow(2.0, params.pitchSt / 12.0);
    const float bodyHz = params.bodyFreqHz;
    const int attackLen = std::max(8, juce::roundToInt(sr * 0.018));
    const double noiseDecay = std::max(1.0, sr * params.tailMs / 1000.0);
    const double spacing = sr * params.clapSpreadMs / 1000.0;
    const float drive = juce::Decibels::decibelsToGain(params.driveDb);
    const float bright = 0.35f + (params.tone + 1.0f) * 0.325f;
    float lastNoise = 0.0f;
    for (int i = 0; i < frames; ++i) {
        const double t = i / sr;
        const float bodyEnv = static_cast<float>(std::exp(-t / (0.045 + params.body * 0.22)));
        const float source = sourceAt(src, a.onsetIndex + i, pitch);
        const float transient = i < attackLen ? static_cast<float>(std::exp(-i / std::max(2.0, attackLen * 0.22))) : 0.0f;
        const float body = std::sin(static_cast<float>(juce::MathConstants<double>::twoPi * bodyHz * t)) * bodyEnv * params.body * 0.55f;
        const float crack = source * transient * params.crack * (0.45f + 0.55f * params.character);
        const float white = rng.next() * 2.0f - 1.0f;
        const float hp = white - lastNoise * (1.0f - bright); lastNoise = white;
        const float noise = hp * static_cast<float>(std::exp(-i / noiseDecay)) * params.noise * 0.38f;
        float clap = 0.0f;
        if (params.mode != 0) {
            for (int c = 0; c < params.clapCount; ++c) {
                const double jitter = (rng.next() - 0.5f) * spacing * 0.32;
                const double local = i - (c * spacing + jitter);
                if (local >= 0) {
                    const float env = static_cast<float>(std::exp(-local / std::max(1.0, sr * (0.010 + c * 0.002))));
                    if (env > 0.001f) {
                        const float texture = sourceAt(src, a.onsetIndex + local * (1.0 + c * 0.013), pitch);
                        clap += (texture * params.character + (rng.next() * 2.0f - 1.0f) * (1.0f - params.character)) * env * 0.24f;
                    }
                }
            }
            const double tailStart = (params.clapCount - 1) * spacing;
            if (i >= tailStart)
                clap += hp * static_cast<float>(std::exp(-(i - tailStart) / std::max(1.0, noiseDecay * 0.75))) * params.noise * 0.22f;
        }
        float x = params.mode == 1 ? clap + crack * 0.35f : (params.mode == 2 ? body + crack + noise + clap * 0.8f : body + crack + noise);
        out[static_cast<size_t>(i)] = std::tanh(x * drive);
    }
    const float trim = juce::Decibels::decibelsToGain(params.trimDb);
    float peak = 0.0f;
    for (auto& x : out) { x = juce::jlimit(-1.0f, 1.0f, x * trim); peak = std::max(peak, std::abs(x)); }
    if (params.normalize && peak > 0.0f) {
        const float gain = std::min(1.0f / peak, 8.0f) * 0.98f;
        for (auto& x : out) x = juce::jlimit(-1.0f, 1.0f, x * gain);
    }
    return out;
}
}
