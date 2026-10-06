#include "ClapRenderer.h"
#include <cmath>
#include <vector>

namespace
{
constexpr float pi=3.14159265358979323846f;
float envExp(float t,float tau){return std::exp(-t/std::max(0.0001f,tau));}
}

void ClapRenderer::render(juce::AudioBuffer<float>& output,
                          const SourceAudio& source,
                          const SnairParameters& p,
                          const LayerMaterial& m,
                          double sampleRate,
                          DeterministicRng& rng)
{
    const int frames=output.getNumSamples();
    std::vector<int> burstStarts;
    const int spread=static_cast<int>(sampleRate*(p.clapSpreadMs/1000.0f));
    for(int b=0;b<p.clapCount;++b)
        burstStarts.push_back(b*spread+rng.integer(0,std::max(1,spread/3)));

    const int burstLen=std::max(8,static_cast<int>(sampleRate*(0.010f+0.018f*p.attack)));
    for(int i=0;i<frames;++i)
    {
        const float t=static_cast<float>(i/sampleRate);
        float y=0.0f;

        for(int b=0;b<p.clapCount;++b)
        {
            const int rel=i-burstStarts[static_cast<size_t>(b)];
            if(rel>=0 && rel<burstLen)
            {
                const float envelope=std::pow(1.0f-static_cast<float>(rel)/burstLen,1.5f);
                const int srcIndex=m.transientSample+static_cast<int>(rel*m.sourceStep)+rng.integer(0,24);
                const float src=LayerExtraction::monoSample(source,srcIndex);
                y+=(m.sourceAmount*src+m.reinforcementAmount*0.75f*rng.bipolar())
                   *envelope*(0.16f+0.20f*p.snap);
            }
        }

        const float tail=envExp(t,m.tailTimeConstant);
        const int srcIndex=m.transientSample+static_cast<int>(i*m.sourceStep);
        const float src=LayerExtraction::monoSample(source,srcIndex);
        y+=(0.5f*rng.bipolar()+0.5f*src)*tail*(0.11f+0.30f*p.texture)*(0.6f+0.4f*p.noise);
        if(m.requiresFallback)
            y+=rng.bipolar()*envExp(t,0.09f)*m.reinforcementAmount*0.05f;

        y+=p.crossBlend*0.12f*std::sin(2.0f*pi*m.bodyFrequencyHz*t)*envExp(t,0.10f);
        y=std::tanh(y*m.drive)/std::tanh(std::max(1.0f,m.drive));
        const float side=0.12f*p.width*rng.bipolar();
        output.setSample(0,i,y*(1.0f-side));
        output.setSample(1,i,y*(1.0f+side));
    }
}
