#include "SourceAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
uint64_t hashSample(uint64_t h, float v)
{
    const auto q = static_cast<int32_t>(std::clamp(v, -1.0f, 1.0f) * 1000000.0f);
    h ^= static_cast<uint32_t>(q);
    h *= 1099511628211ull;
    return h;
}
}

bool SourceAnalyzer::analyze(const SourceAudio& source, SourceAnalysis& r, juce::String& error)
{
    error.clear();
    if (source.samples.getNumSamples() <= 1 || source.samples.getNumChannels() <= 0 || source.sampleRate <= 0.0)
    {
        error = "Source is too short to analyze.";
        return false;
    }

    r = {};
    r.durationSeconds = source.durationSeconds();
    r.sampleRate = source.sampleRate;
    r.channels = source.channelCount;

    const int n = source.samples.getNumSamples();
    double sumSq = 0.0;
    double low = 0.0, mid = 0.0, high = 0.0;
    int crossings = 0;
    float prev = 0.0f;
    float maxFlux = -1.0f;
    int maxFluxPos = 0;
    uint64_t fingerprint = 1469598103934665603ull;
    std::vector<float> mono(static_cast<size_t>(n));

    for (int i=0;i<n;++i)
    {
        float x=0.0f;
        for(int ch=0;ch<source.samples.getNumChannels();++ch) x += source.samples.getSample(ch,i);
        x /= static_cast<float>(source.samples.getNumChannels());
        mono[static_cast<size_t>(i)] = x;
        r.peak = std::max(r.peak, std::abs(x));
        sumSq += static_cast<double>(x)*x;
        if ((x >= 0.0f) != (prev >= 0.0f)) ++crossings;
        prev = x;
        if ((i & 63)==0) fingerprint = hashSample(fingerprint,x);
    }

    r.rms = static_cast<float>(std::sqrt(sumSq / std::max(1,n)));
    if (r.rms < 1.0e-7f || r.peak < 1.0e-6f)
    {
        error = "The source is effectively silent. Load a source with audible material.";
        return false;
    }
    r.crest = r.peak / std::max(1.0e-7f,r.rms);
    r.zeroCrossingRate = static_cast<float>(crossings) / static_cast<float>(n);
    r.fingerprint = fingerprint;

    const int hop = std::max(16, static_cast<int>(source.sampleRate * 0.004));
    const int win = std::max(hop*2, static_cast<int>(source.sampleRate * 0.012));
    float previousEnergy = 0.0f;
    for (int start=0; start+win<n; start+=hop)
    {
        double e=0.0;
        for(int i=start;i<start+win;++i){ const float x=mono[static_cast<size_t>(i)]; e += x*x; }
        const float energy = static_cast<float>(e/win);
        const float flux = std::max(0.0f, energy-previousEnergy);
        if (flux > maxFlux){ maxFlux=flux; maxFluxPos=start; }
        if (flux > std::max(1.0e-8f, r.rms*r.rms*0.35f))
            r.transientCandidates.push_back(start);
        previousEnergy=energy;
    }
    r.strongestTransientSample = maxFluxPos;
    if (r.transientCandidates.empty()) r.transientCandidates.push_back(maxFluxPos);
    if (r.transientCandidates.size()>32) r.transientCandidates.resize(32);

    const int fftOrder=11, fftSize=1<<fftOrder;
    juce::dsp::FFT fft(fftOrder);
    std::vector<float> data(static_cast<size_t>(fftSize*2),0.0f);
    const int centre=std::clamp(maxFluxPos,0,std::max(0,n-1));
    const int begin=std::max(0,centre-fftSize/4);
    for(int i=0;i<fftSize && begin+i<n;++i) data[static_cast<size_t>(i)] = mono[static_cast<size_t>(begin+i)];
    fft.performFrequencyOnlyForwardTransform(data.data());

    double weighted=0.0,total=0.0;
    const double binHz=source.sampleRate/fftSize;
    for(int b=1;b<fftSize/2;++b)
    {
        const double mag=std::max(0.0f,data[static_cast<size_t>(b)]);
        const double hz=b*binHz;
        total+=mag; weighted+=mag*hz;
        if(hz<250.0) low+=mag; else if(hz<2500.0) mid+=mag; else high+=mag;
    }
    r.spectralCentroidHz = total>0.0 ? static_cast<float>(weighted/total) : 0.0f;
    const double bandTotal=std::max(1.0e-12,low+mid+high);
    r.lowEnergyRatio=static_cast<float>(low/bandTotal);
    r.midEnergyRatio=static_cast<float>(mid/bandTotal);
    r.highEnergyRatio=static_cast<float>(high/bandTotal);
    r.noisyEstimate=std::clamp(r.zeroCrossingRate*4.0f + r.highEnergyRatio*0.45f,0.0f,1.0f);

    int bestBin=1; float bestMag=0.0f;
    const int minBin=std::max(1,static_cast<int>(70.0/binHz));
    const int maxBin=std::min(fftSize/2-1,static_cast<int>(450.0/binHz));
    for(int b=minBin;b<=maxBin;++b) if(data[static_cast<size_t>(b)]>bestMag){bestMag=data[static_cast<size_t>(b)];bestBin=b;}
    r.dominantBodyHz=static_cast<float>(std::clamp(bestBin*binHz,70.0,450.0));
    return true;
}
