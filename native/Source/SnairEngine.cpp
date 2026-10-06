#include "SnairEngine.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace
{
constexpr float pi=3.14159265358979323846f;
float envExp(float t,float tau){ return std::exp(-t/std::max(0.0001f,tau)); }

struct Rng
{
    explicit Rng(uint32_t s):g(s){}
    float bipolar(){ return std::uniform_real_distribution<float>(-1.0f,1.0f)(g); }
    float uni(){ return std::uniform_real_distribution<float>(0.0f,1.0f)(g); }
    int integer(int lo,int hi){ return std::uniform_int_distribution<int>(lo,hi)(g); }
    std::mt19937 g;
};

float sourceAt(const SourceAudio& s,int i,int ch)
{
    if(s.samples.getNumSamples()==0) return 0.0f;
    i=std::clamp(i,0,s.samples.getNumSamples()-1);
    ch=std::clamp(ch,0,s.samples.getNumChannels()-1);
    return s.samples.getSample(ch,i);
}

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
        auto* d=buffer.getWritePointer(ch);
        for(int i=0;i<buffer.getNumSamples();++i)
        {
            const float x=d[i];
            lp += alpha*(x-lp);
            d[i]=tone<0.0f ? juce::jmap(-tone,x,lp) : x+(x-lp)*(0.42f*tone);
        }
    }
}
}

std::shared_ptr<RenderedHit> SnairEngine::render(const SourceAudio& source,
                                                  const SourceAnalysis& a,
                                                  SnairParameters p,
                                                  uint64_t generationId,
                                                  juce::String& error,
                                                  double outputSampleRate)
{
    error.clear(); p.sanitize();
    if(source.samples.getNumSamples()<2 || source.sampleRate<=0.0)
    {
        error="No valid source is loaded.";
        return {};
    }

    const double sr=(std::isfinite(outputSampleRate) && outputSampleRate>=8000.0)
        ? outputSampleRate : source.sampleRate;
    const float requestedTail=std::clamp(p.tailMs/1000.0f,0.02f,2.0f);
    const float sizeTail=0.08f+0.58f*p.size;
    const float duration=std::clamp(std::max(requestedTail,sizeTail),0.06f,2.0f);
    const int frames=std::max(2,static_cast<int>(std::ceil(duration*sr)));

    auto hit=std::make_shared<RenderedHit>();
    hit->samples.setSize(2,frames);
    hit->samples.clear();
    hit->sampleRate=sr;
    hit->sourceFingerprint=a.fingerprint;
    hit->seed=p.seed;
    hit->generationId=generationId;
    hit->usedFallback=(a.crest<1.35f || a.transientCandidates.empty());

    Rng rng(p.seed ^ static_cast<uint32_t>(a.fingerprint));
    const int transient=std::clamp(static_cast<int>(a.strongestTransientSample),0,source.samples.getNumSamples()-1);
    const float pitchRatio=std::pow(2.0f,p.pitchSt/12.0f);
    const double sourceStep=(source.sampleRate/sr)*pitchRatio;
    const float bodyHz=std::clamp(0.55f*p.bodyFreqHz+0.45f*a.dominantBodyHz,70.0f,450.0f);
    const float sourceAmt=0.10f+0.75f*p.sourceCharacter;
    const float synthAmt=0.85f-0.65f*p.sourceCharacter;
    const float drive=1.0f+3.0f*p.dirt+0.07f*p.driveDb;
    const float tailTau=std::clamp(requestedTail*(0.34f+0.40f*p.size),0.015f,1.2f);

    if(p.mode==SnairMode::snare)
    {
        const int attackFrames=std::max(8,static_cast<int>(sr*(0.006+0.030*p.attack)));
        for(int i=0;i<frames;++i)
        {
            const float t=static_cast<float>(i/sr);
            const float attackEnv=i<attackFrames ? (1.0f-static_cast<float>(i)/attackFrames) : 0.0f;
            const int srcIndex=transient+static_cast<int>(i*sourceStep);
            const float src=0.5f*(sourceAt(source,srcIndex,0)+sourceAt(source,srcIndex,source.samples.getNumChannels()-1));
            const float crack=src*attackEnv*sourceAmt*(0.55f+0.75f*p.punch)*(0.72f+0.38f*p.snap);
            const float bodyEnv=envExp(t,0.045f+0.22f*p.size+0.10f*p.body);
            const float body=std::sin(2.0f*pi*bodyHz*t+0.22f*std::sin(2*pi*bodyHz*1.8f*t))
                           *bodyEnv*synthAmt*(0.18f+0.42f*p.body);
            const float noiseEnv=envExp(t,tailTau);
            const float tex=(0.55f*rng.bipolar()+0.45f*src)*noiseEnv
                           *(0.08f+0.34f*p.texture)*(0.55f+0.45f*p.noise);
            const float fallback=hit->usedFallback
                ? rng.bipolar()*attackEnv*synthAmt*0.08f : 0.0f;
            const float opposite=p.crossBlend*0.10f*rng.bipolar()*envExp(t,0.08f);
            float y=(crack+body+tex+fallback+opposite)*(0.70f+0.55f*p.punch);
            y=std::tanh(y*drive)/std::tanh(std::max(1.0f,drive));
            const float panSpread=0.10f+0.55f*p.width;
            hit->samples.setSample(0,i,y*(1.0f-panSpread*0.10f));
            hit->samples.setSample(1,i,y*(1.0f+panSpread*0.10f));
        }
    }
    else
    {
        std::vector<int> burstStarts;
        const int spread=static_cast<int>(sr*(p.clapSpreadMs/1000.0f));
        for(int b=0;b<p.clapCount;++b)
            burstStarts.push_back(b*spread+rng.integer(0,std::max(1,spread/3)));

        const int burstLen=std::max(8,static_cast<int>(sr*(0.010f+0.018f*p.attack)));
        for(int i=0;i<frames;++i)
        {
            const float t=static_cast<float>(i/sr);
            float y=0.0f;
            for(int b=0;b<p.clapCount;++b)
            {
                const int rel=i-burstStarts[static_cast<size_t>(b)];
                if(rel>=0 && rel<burstLen)
                {
                    const float e=std::pow(1.0f-static_cast<float>(rel)/burstLen,1.5f);
                    const int srcIndex=transient+static_cast<int>(rel*sourceStep)+rng.integer(0,24);
                    const float src=0.5f*(sourceAt(source,srcIndex,0)+sourceAt(source,srcIndex,source.samples.getNumChannels()-1));
                    y += (sourceAmt*src+synthAmt*0.75f*rng.bipolar())*e*(0.16f+0.20f*p.snap);
                }
            }
            const float tail=envExp(t,tailTau);
            const int srcIndex=transient+static_cast<int>(i*sourceStep);
            const float src=0.5f*(sourceAt(source,srcIndex,0)+sourceAt(source,srcIndex,source.samples.getNumChannels()-1));
            y += (0.5f*rng.bipolar()+0.5f*src)*tail*(0.11f+0.30f*p.texture)*(0.6f+0.4f*p.noise);
            if(hit->usedFallback) y += rng.bipolar()*envExp(t,0.09f)*synthAmt*0.05f;
            y += p.crossBlend*0.12f*std::sin(2*pi*bodyHz*t)*envExp(t,0.10f);
            y=std::tanh(y*drive)/std::tanh(std::max(1.0f,drive));
            const float side=0.12f*p.width*rng.bipolar();
            hit->samples.setSample(0,i,y*(1.0f-side));
            hit->samples.setSample(1,i,y*(1.0f+side));
        }
    }

    applyTone(hit->samples,p.tone,sr);

    const int fade=std::min(frames/2,std::max(1,static_cast<int>(sr*0.002)));
    for(int ch=0;ch<2;++ch)
    {
        auto* d=hit->samples.getWritePointer(ch);
        for(int i=0;i<frames;++i)
        {
            if(!std::isfinite(d[i])) d[i]=0.0f;
            if(i<fade) d[i]*=static_cast<float>(i)/fade;
            if(i>=frames-fade) d[i]*=static_cast<float>(frames-1-i)/fade;
        }
    }

    float peak=hit->samples.getMagnitude(0,frames);
    if(p.normalizeRender && peak>1.0e-6f)
    {
        hit->samples.applyGain(0.98f/peak);
        peak=hit->samples.getMagnitude(0,frames);
    }
    if(peak>1.0f) hit->samples.applyGain(0.98f/peak);
    hit->peak=hit->samples.getMagnitude(0,frames);
    return hit;
}

SnairParameters SnairEngine::randomized(SnairParameters p,uint32_t s)
{
    Rng r(s); p.seed=s;
    p.sourceCharacter=0.15f+0.75f*r.uni(); p.punch=0.2f+0.8f*r.uni(); p.snap=0.2f+0.8f*r.uni();
    p.body=0.15f+0.8f*r.uni(); p.texture=0.1f+0.9f*r.uni(); p.dirt=0.05f+0.75f*r.uni();
    p.size=0.15f+0.8f*r.uni(); p.bodyFreqHz=90.0f+280.0f*r.uni(); p.pitchSt=-8.0f+16.0f*r.uni();
    p.tone=-0.7f+1.4f*r.uni(); p.driveDb=1.0f+14.0f*r.uni(); p.width=0.15f+0.85f*r.uni();
    p.clapCount=r.integer(2,6); p.clapSpreadMs=9.0f+24.0f*r.uni(); p.tailMs=80.0f+720.0f*r.uni();
    p.attack=0.15f+0.8f*r.uni(); p.noise=0.15f+0.8f*r.uni(); p.crossBlend=0.35f*r.uni();
    p.sanitize(); return p;
}

SnairParameters SnairEngine::mutated(SnairParameters p,uint32_t s)
{
    Rng r(s); auto m=[&](float v,float amt){return v+r.bipolar()*amt;};
    p.seed=s; p.sourceCharacter=m(p.sourceCharacter,0.10f); p.punch=m(p.punch,0.12f); p.snap=m(p.snap,0.12f);
    p.body=m(p.body,0.10f); p.texture=m(p.texture,0.12f); p.dirt=m(p.dirt,0.10f); p.size=m(p.size,0.10f);
    p.bodyFreqHz=m(p.bodyFreqHz,35.0f); p.pitchSt=m(p.pitchSt,2.5f); p.tone=m(p.tone,0.15f);
    p.driveDb=m(p.driveDb,2.5f); p.width=m(p.width,0.12f); p.tailMs=m(p.tailMs,90.0f);
    p.attack=m(p.attack,0.10f); p.noise=m(p.noise,0.10f); p.crossBlend=m(p.crossBlend,0.08f);
    if(r.uni()>0.65f) p.clapCount += r.uni()>0.5f?1:-1;
    p.clapSpreadMs=m(p.clapSpreadMs,3.0f); p.sanitize(); return p;
}
