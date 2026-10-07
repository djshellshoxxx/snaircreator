#include "HitPlayer.h"
#include <cmath>

void HitPlayer::noteOn(float velocity, int note, bool keyTrack) noexcept
{
    const auto* hit = getHit();
    if (hit == nullptr || hit->samples.getNumSamples() == 0) return;
    // Voice cap: reuse a free voice, else steal the oldest (predictable).
    Voice* target = nullptr;
    for (auto& v : voices) if (v.hit == nullptr) { target = &v; break; }
    if (target == nullptr)
    {
        target = &voices[0];
        for (auto& v : voices) if (v.age < target->age) target = &v;
    }
    target->hit = hit;
    target->pos = 0.0;
    target->rate = (hit->sampleRate / outputRate) * (keyTrack ? std::pow(2.0, (note - 60) / 12.0) : 1.0);
    target->gain = juce::jlimit(0.0f, 1.0f, velocity);
    target->age = ++counter;
}

void HitPlayer::render(juce::AudioBuffer<float>& out, int start, int num, float g0, float g1) noexcept
{
    const int outCh = out.getNumChannels();
    if (outCh == 0 || num <= 0) return;
    for (auto& v : voices)
    {
        if (v.hit == nullptr) continue;
        const int len = v.hit->samples.getNumSamples();
        const float* L = v.hit->samples.getReadPointer(0);
        const float* R = v.hit->samples.getReadPointer(1);
        for (int i = 0; i < num; ++i)
        {
            const int k = static_cast<int>(v.pos);
            if (k >= len - 1) { v.hit = nullptr; break; }
            const float f = static_cast<float>(v.pos - k);
            const float g = v.gain * (g0 + (g1 - g0) * static_cast<float>(i) / static_cast<float>(num));
            const float l = (L[k] + f * (L[k + 1] - L[k])) * g;
            const float r = (R[k] + f * (R[k + 1] - R[k])) * g;
            if (outCh == 1) out.addSample(0, start + i, 0.5f * (l + r));
            else { out.addSample(0, start + i, l); out.addSample(1, start + i, r); }
            v.pos += v.rate;
        }
    }
}

int HitPlayer::activeVoiceCount() const noexcept
{
    int n = 0;
    for (const auto& v : voices) n += v.hit != nullptr ? 1 : 0;
    return n;
}

void HitPlayer::publishInUse(std::array<std::atomic<const RenderedHit*>, maxVoices + 1>& table) const noexcept
{
    for (size_t i = 0; i < voices.size(); ++i) table[i].store(voices[i].hit, std::memory_order_release);
    table[maxVoices].store(getHit(), std::memory_order_release);
}
