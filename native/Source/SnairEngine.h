#pragma once
#include <JuceHeader.h>
#include <vector>

namespace snair {
struct Analysis {
    int frames = 0;
    double duration = 0.0;
    float peak = 0.0f;
    float rms = 0.0f;
    float zeroCrossingRate = 0.0f;
    int onsetIndex = 0;
    float bodyFreqHint = 185.0f;
};

struct Params {
    int mode = 0; // 0 snare, 1 clap, 2 hybrid
    int seed = 1;
    float character = 0.72f, body = 0.68f, bodyFreqHz = 185.0f, crack = 0.72f, noise = 0.62f;
    float tailMs = 260.0f, clapSpreadMs = 18.0f, width = 0.35f, driveDb = 4.0f, tone = 0.0f;
    float pitchSt = 0.0f, trimDb = 0.0f, outputMs = 420.0f;
    int clapCount = 4;
    bool normalize = true;
};

class SnairEngine {
public:
    static Analysis analyse(const std::vector<float>& samples, double sampleRate);
    static Params sanitize(Params p);
    static std::vector<float> render(const std::vector<float>& samples, double sampleRate, Params params);
};
}
