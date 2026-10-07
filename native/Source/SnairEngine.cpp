#include "SnairEngine.h"
#include "ClapRenderer.h"
#include "DeterministicRng.h"
#include "LayerExtraction.h"
#include "SnareRenderer.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr float pi=3.14159265358979323846f;

void applyTone(juce::AudioBuffer<float>& buffer,float tone,double sampleRate)
{
    if(std::abs(tone)<1.0e-5f) return;
    const float cutoff=tone<0.0f
        ? juce::jmap(tone,-1.0f,0.0f,1700.0f,8500.0f)
        : juce::jmap(tone,0.0f,1.0f,8500.0f,16000.0f);
    const float alpha=1.0f-std::exp(-2.0f*pi*cutoff/static_cast<float>(sampleRate));

    for(int ch=0;ch<buffer.getNumChannels();++ch)
    {
        float lp=0.0f;
        auto* data=buffer.getWritePointer(ch);
        for(int i=0;i<buffer.getNumSamples();++i)
        {
            const float x=data[i];
            lp+=alpha*(x-lp);
            data[i]=tone<0.0f ? juce::jmap(-tone,x,lp) : x+(x-lp)*(0.42f*tone);
        }
    }
}

void applySafety(juce::AudioBuffer<float>& buffer,double sampleRate,bool normalize,float& peak)
{
    const int frames=buffer.getNumSamples();
    const int fade=std::min(frames/2,std::max(1,static_cast<int>(sampleRate*0.002)));

    for(int ch=0;ch<buffer.getNumChannels();++ch)
    {
        auto* data=buffer.getWritePointer(ch);
        for(int i=0;i<frames;++i)
        {
            if(!std::isfinite(data[i])) data[i]=0.0f;
            if(i<fade) data[i]*=static_cast<float>(i)/fade;
            if(i>=frames-fade) data[i]*=static_cast<float>(frames-1-i)/fade;
        }
    }

    peak=buffer.getMagnitude(0,frames);
    if(normalize && peak>1.0e-6f)
    {
        buffer.applyGain(0.98f/peak);
        peak=buffer.getMagnitude(0,frames);
    }

    if(peak>1.0f)
    {
        buffer.applyGain(0.98f/peak);
        peak=buffer.getMagnitude(0,frames);
    }
}
}

std::shared_ptr<RenderedHit> SnairEngine::render(const SourceAudio& source,
                                                  const SourceAnalysis& analysis,
                                                  SnairParameters p,
                                                  uint64_t generationId,
                                                  juce::String& error,
                                                  double outputSampleRate)
{
    error.clear();
    p.sanitize();

    if(source.samples.getNumSamples()<2 || source.sampleRate<=0.0)
    {
        error="No valid source is loaded.";
        return {};
    }

    const double sampleRate=(std::isfinite(outputSampleRate) && outputSampleRate>=8000.0)
        ? outputSampleRate : source.sampleRate;
    const float requestedTail=std::clamp(p.tailMs/1000.0f,0.02f,2.0f);
    const float sizeTail=0.08f+0.58f*p.size;
    const float duration=std::clamp(std::max(requestedTail,sizeTail),0.06f,2.0f);
    const int frames=std::max(2,static_cast<int>(std::ceil(duration*sampleRate)));

    auto hit=std::make_shared<RenderedHit>();
    hit->samples.setSize(2,frames);
    hit->samples.clear();
    hit->sampleRate=sampleRate;
    hit->sourceFingerprint=analysis.fingerprint;
    hit->seed=p.seed;
    hit->generationId=generationId;

    const auto material=LayerExtraction::prepare(source,analysis,p,sampleRate);
    hit->usedFallback=material.requiresFallback;
    DeterministicRng rng(p.seed^static_cast<uint32_t>(analysis.fingerprint));

    if(p.mode==SnairMode::snare)
        SnareRenderer::render(hit->samples,source,p,material,sampleRate,rng);
    else
        ClapRenderer::render(hit->samples,source,p,material,sampleRate,rng);

    applyTone(hit->samples,p.tone,sampleRate);
    applySafety(hit->samples,sampleRate,p.normalizeRender,hit->peak);
    return hit;
}

SnairParameters SnairEngine::randomized(SnairParameters p,uint32_t seed)
{
    DeterministicRng rng(seed);
    p.seed=seed;
    p.sourceCharacter=0.15f+0.75f*rng.uniform();
    p.punch=0.2f+0.8f*rng.uniform();
    p.snap=0.2f+0.8f*rng.uniform();
    p.body=0.15f+0.8f*rng.uniform();
    p.texture=0.1f+0.9f*rng.uniform();
    p.dirt=0.05f+0.75f*rng.uniform();
    p.size=0.15f+0.8f*rng.uniform();
    p.bodyFreqHz=90.0f+280.0f*rng.uniform();
    p.pitchSt=-8.0f+16.0f*rng.uniform();
    p.tone=-0.7f+1.4f*rng.uniform();
    p.driveDb=1.0f+14.0f*rng.uniform();
    p.width=0.15f+0.85f*rng.uniform();
    p.clapCount=rng.integer(2,6);
    p.clapSpreadMs=9.0f+24.0f*rng.uniform();
    p.tailMs=80.0f+720.0f*rng.uniform();
    p.attack=0.15f+0.8f*rng.uniform();
    p.noise=0.15f+0.8f*rng.uniform();
    p.crossBlend=0.35f*rng.uniform();
    p.sanitize();
    return p;
}

SnairParameters SnairEngine::mutated(SnairParameters p,uint32_t seed)
{
    DeterministicRng rng(seed);
    auto mutate=[&](float value,float amount){return value+rng.bipolar()*amount;};
    p.seed=seed;
    p.sourceCharacter=mutate(p.sourceCharacter,0.10f);
    p.punch=mutate(p.punch,0.12f);
    p.snap=mutate(p.snap,0.12f);
    p.body=mutate(p.body,0.10f);
    p.texture=mutate(p.texture,0.12f);
    p.dirt=mutate(p.dirt,0.10f);
    p.size=mutate(p.size,0.10f);
    p.bodyFreqHz=mutate(p.bodyFreqHz,35.0f);
    p.pitchSt=mutate(p.pitchSt,2.5f);
    p.tone=mutate(p.tone,0.15f);
    p.driveDb=mutate(p.driveDb,2.5f);
    p.width=mutate(p.width,0.12f);
    p.tailMs=mutate(p.tailMs,90.0f);
    p.attack=mutate(p.attack,0.10f);
    p.noise=mutate(p.noise,0.10f);
    p.crossBlend=mutate(p.crossBlend,0.08f);
    if(rng.uniform()>0.65f) p.clapCount+=rng.uniform()>0.5f?1:-1;
    p.clapSpreadMs=mutate(p.clapSpreadMs,3.0f);
    p.sanitize();
    return p;
}
