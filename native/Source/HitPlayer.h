#pragma once
#include "RenderedHit.h"
#include <array>
#include <atomic>

// Realtime-safe one-shot player. Voices hold raw pointers to immutable hits; the owner guarantees
// lifetime via the in-use table + block counter protocol (see PluginProcessor::collectGarbage).
class HitPlayer
{
public:
    static constexpr int maxVoices = 16;

    void prepare(double sampleRate) noexcept { outputRate = sampleRate; }
    void setHit(const RenderedHit* h) noexcept { active.store(h, std::memory_order_release); }
    const RenderedHit* getHit() const noexcept { return active.load(std::memory_order_acquire); }

    // Starts a voice at the given sample offset. Velocity 0..1. Note used only when key tracking.
    void noteOn(float velocity, int note, bool keyTrack) noexcept;
    void allNotesOff() noexcept { for (auto& v : voices) v.hit = nullptr; }
    // Adds playback into the buffer region [start, start+num).
    void render(juce::AudioBuffer<float>& out, int start, int num, float gainStart, float gainEnd) noexcept;
    int activeVoiceCount() const noexcept;
    // Publishes pointers currently referenced by voices (for safe deferred release).
    void publishInUse(std::array<std::atomic<const RenderedHit*>, maxVoices + 1>& table) const noexcept;

private:
    struct Voice { const RenderedHit* hit = nullptr; double pos = 0.0; double rate = 1.0; float gain = 0.0f; uint64_t age = 0; };
    std::array<Voice, maxVoices> voices {};
    std::atomic<const RenderedHit*> active { nullptr };
    double outputRate = 48000.0;
    uint64_t counter = 0;
};
