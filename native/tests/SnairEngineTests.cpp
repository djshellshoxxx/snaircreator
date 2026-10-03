#include <JuceHeader.h>
#include "../Source/SnairEngine.h"
#include <cmath>
#include <iostream>

static std::vector<float> source() {
    std::vector<float> x(4800, 0.0f); x[20] = 1.0f;
    for (size_t i = 21; i < 700; ++i) x[i] = std::sin(static_cast<float>(i) * .17f) * std::exp(-static_cast<float>(i) / 300.0f);
    return x;
}

static int fail(const char* message) { std::cerr << "FAIL: " << message << '\n'; return 1; }

int main() {
    auto s = source();
    snair::Params p; p.seed = 42; p.mode = 2; p.outputMs = 180;
    auto a = snair::SnairEngine::render(s, 48000, p);
    auto b = snair::SnairEngine::render(s, 48000, p);
    if (a != b) return fail("same seed is not deterministic");
    if (a.empty()) return fail("render is empty");
    for (auto v : a) if (!std::isfinite(v) || std::abs(v) > 1.0001f) return fail("output is non-finite or out of range");
    p.mode = 1; p.width = 0.0f; auto narrow = snair::SnairEngine::render(s, 48000, p);
    p.width = 1.0f; auto wide = snair::SnairEngine::render(s, 48000, p);
    double delta = 0; for (size_t i = 0; i < narrow.size(); ++i) delta += std::abs(narrow[i] - wide[i]);
    if (delta <= .001) return fail("width does not alter clap texture");
    std::cout << "SnairEngine native tests passed\n";
    return 0;
}
