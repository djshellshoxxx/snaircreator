#pragma once
#include <cstdint>

// Platform-independent PRNG (SplitMix64). std:: distributions are implementation-defined,
// so all render randomness goes through this class to keep renders identical across OSes.
class DeterministicRng
{
public:
    explicit DeterministicRng(uint64_t seed) noexcept : state(seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) {}

    uint64_t next() noexcept
    {
        uint64_t z = (state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    float uniform() noexcept { return static_cast<float>(next() >> 40) * (1.0f / 16777216.0f); }
    float bipolar() noexcept { return uniform() * 2.0f - 1.0f; }
    int integer(int low, int high) noexcept // inclusive
    {
        if (high <= low) return low;
        return low + static_cast<int>(next() % static_cast<uint64_t>(high - low + 1));
    }

private:
    uint64_t state;
};
