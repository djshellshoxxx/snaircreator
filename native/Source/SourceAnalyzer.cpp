#include "SourceAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace
{
uint64_t hashSample(uint64_t h,float v)
{
    const auto q=static_cast<int32_t>(std::clamp(v,-1.0f,1.0f)*1000000.0f);
    h^=static_cast<uint32_t>(q);
    h*=1099511628211ull;
    return h;
}
}

bool SourceAnalyzer::analyze(const SourceAudio& source,SourceAnalysis& r,juce::String& error)
{
    error.clear();
    if(source.samples.getNumSamples()<=1 || source.samples.getNumChannels()<=0 || source.sampleRate<=0.0)
    {
        error="Source is too short to analyze.";
        return false;
    }

    r={};
    r.durationSeconds=source.durationSeconds();
    r.sampleRate=source.sampleRate;
    r.channels=source.channelCount;
    r.fileSizeBytes=source.fileSizeBytes;

    const int n=source.samples.getNumSamples();
    std::vector<float> mono(static_cast<size_t>(n),0.0f);
    double mean=0.0;
    for(int i=0;i<n;++i)
    {
        double x=0.0;
        for(int ch=0;ch<source.samples.getNumChannels();++ch)
            x+=source.samples.getSample(ch,i);
        x/=static_cast<double>(source.samples.getNumChannels());
        mono[static_cast<size_t>(i)]=static_cast<float>(x);
        mean+=x;
    }
    mean/=static_cast<double>(n);
    r.dcOffset=static_cast<float>(mean);

    double sumSq=0.0;
    int crossings=0;
    float prev=mono.front()-r.dcOffset;
    uint64_t fingerprint=1469598103934665603ull;
    for(int i=0;i<n;++i)
    {
        const float x=mono[static_cast<size_t>(i)]-r.dcOffset;
        mono[static_cast<size_t>(i)]=x;
        r.peak=std::max(r.peak,std::abs(x));
        sumSq+=static_cast<double>(x)*x;
        if(i>0 && ((x>=0.0f)!=(prev>=0.0f))) ++crossings;
        prev=x;
        if((i&63)==0) fingerprint=hashSample(fingerprint,x);
    }

    r.rms=static_cast<float>(std::sqrt(sumSq/std::max(1,n)));
    if(r.rms<1.0e-7f || r.peak<1.0e-6f)
    {
        error="The source is effectively silent. Load a source with audible material.";
        return false;
    }

    r.crest=r.peak/std::max(1.0e-7f,r.rms);
    r.zeroCrossingRate=static_cast<float>(crossings)/static_cast<float>(std::max(1,n-1));
    r.fingerprint=fingerprint;

    const int hop=std::max(16,static_cast<int>(source.sampleRate*0.004));
    const int win=std::max(hop*2,static_cast<int>(source.sampleRate*0.012));
    float previousEnergy=0.0f;
    float maxFlux=-1.0f;
    int maxFluxPos=0;
    std::vector<std::pair<float,int>> candidates;

    for(int start=0;start+win<n;start+=hop)
    {
        double e=0.0;
        for(int i=start;i<start+win;++i)
        {
            const float x=mono[static_cast<size_t>(i)];
            e+=x*x;
        }

        const float energy=static_cast<float>(e/win);
        const float flux=std::max(0.0f,energy-previousEnergy);
        if(flux>maxFlux)
        {
            maxFlux=flux;
            maxFluxPos=start;
        }

        if(flux>std::max(1.0e-8f,r.rms*r.rms*0.35f))
            candidates.emplace_back(flux,start);
        previousEnergy=energy;
    }

    std::stable_sort(candidates.begin(),candidates.end(),
        [](const auto& a,const auto& b){return a.first>b.first;});
    if(candidates.empty()) candidates.emplace_back(std::max(0.0f,maxFlux),maxFluxPos);
    if(candidates.size()>32) candidates.resize(32);
    std::sort(candidates.begin(),candidates.end(),
        [](const auto& a,const auto& b){return a.second<b.second;});
    for(const auto& item:candidates) r.transientCandidates.push_back(item.second);

    r.strongestTransientSample=maxFluxPos;
    r.transientDensityPerSecond=static_cast<float>(
        static_cast<double>(r.transientCandidates.size())/std::max(0.001,r.durationSeconds));

    const int attackPre=static_cast<int>(source.sampleRate*0.002);
    const int attackPost=static_cast<int>(source.sampleRate*0.040);
    r.attackWindowStart=std::max<int64_t>(0,r.strongestTransientSample-attackPre);
    r.attackWindowEnd=std::min<int64_t>(n,r.strongestTransientSample+attackPost);
    r.tailWindowStart=std::min<int64_t>(n,r.attackWindowEnd);
    r.tailWindowEnd=std::min<int64_t>(n,r.tailWindowStart+static_cast<int64_t>(source.sampleRate*0.75));

    constexpr int fftOrder=11;
    constexpr int fftSize=1<<fftOrder;
    juce::dsp::FFT fft(fftOrder);
    std::vector<float> data(static_cast<size_t>(fftSize*2),0.0f);

    const int centre=std::clamp(maxFluxPos,0,std::max(0,n-1));
    const int begin=std::max(0,centre-fftSize/4);
    for(int i=0;i<fftSize && begin+i<n;++i)
    {
        const float w=0.5f-0.5f*std::cos(2.0f*juce::MathConstants<float>::pi
                                       *static_cast<float>(i)/static_cast<float>(fftSize-1));
        data[static_cast<size_t>(i)]=mono[static_cast<size_t>(begin+i)]*w;
    }
    fft.performFrequencyOnlyForwardTransform(data.data());

    const double binHz=source.sampleRate/fftSize;
    double weighted=0.0,total=0.0,low=0.0,mid=0.0,high=0.0;
    double logSum=0.0;
    int spectralBins=0;
    std::vector<double> magnitudes(static_cast<size_t>(fftSize/2),0.0);

    for(int b=1;b<fftSize/2;++b)
    {
        const double mag=std::max(1.0e-12,static_cast<double>(data[static_cast<size_t>(b)]));
        magnitudes[static_cast<size_t>(b)]=mag;
        const double hz=b*binHz;
        total+=mag;
        weighted+=mag*hz;
        logSum+=std::log(mag);
        ++spectralBins;
        if(hz<250.0) low+=mag;
        else if(hz<2500.0) mid+=mag;
        else high+=mag;
    }

    r.spectralCentroidHz=total>0.0?static_cast<float>(weighted/total):0.0f;
    const double arithmetic=total/std::max(1,spectralBins);
    const double geometric=std::exp(logSum/std::max(1,spectralBins));
    r.spectralFlatness=arithmetic>0.0
        ? static_cast<float>(std::clamp(geometric/arithmetic,0.0,1.0)) : 0.0f;

    const double rollTarget=total*0.85;
    double cumulative=0.0;
    int rollBin=1;
    for(int b=1;b<fftSize/2;++b)
    {
        cumulative+=magnitudes[static_cast<size_t>(b)];
        if(cumulative>=rollTarget)
        {
            rollBin=b;
            break;
        }
    }
    r.spectralRolloffHz=static_cast<float>(rollBin*binHz);

    const double bandTotal=std::max(1.0e-12,low+mid+high);
    r.lowEnergyRatio=static_cast<float>(low/bandTotal);
    r.midEnergyRatio=static_cast<float>(mid/bandTotal);
    r.highEnergyRatio=static_cast<float>(high/bandTotal);
    r.noisyEstimate=std::clamp(
        r.zeroCrossingRate*2.0f+r.highEnergyRatio*0.35f+r.spectralFlatness*0.45f,
        0.0f,1.0f);

    int bestBin=1;
    float bestMag=0.0f;
    const int minBin=std::max(1,static_cast<int>(70.0/binHz));
    const int maxBin=std::min(fftSize/2-1,static_cast<int>(450.0/binHz));
    for(int b=minBin;b<=maxBin;++b)
    {
        const auto mag=data[static_cast<size_t>(b)];
        if(mag>bestMag)
        {
            bestMag=mag;
            bestBin=b;
        }
    }
    r.dominantBodyHz=static_cast<float>(std::clamp(bestBin*binHz,70.0,450.0));
    return true;
}
