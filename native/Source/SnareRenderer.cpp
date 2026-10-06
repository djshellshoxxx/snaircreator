#include "SnareRenderer.h"
#include <cmath>

namespace
{
constexpr float pi=3.14159265358979323846f;
float envExp(float t,float tau){return std::exp(-t/std::max(0.0001f,tau));}
}

void SnareRenderer::render(juce::AudioBuffer<float>& output,
                           const SourceAudio& source,
                           const SnairParameters& p,
                           const LayerMaterial& m,
                           double sampleRate,
                           DeterministicRng& rng)
{
    const int frames=output.getNumSamples();
    const int attackFrames=std::max(8,static_cast<int>(sampleRate*(0.006+0.030*p.attack)));

    for(int i=0;i<frames;++i)
    {
        const float t=static_cast<float>(i/sampleRate);
        const float attackEnv=i<attackFrames ? (1.0f-static_cast<float>(i)/attackFrames) : 0.0f;
        const int srcIndex=m.transientSample+static_cast<int>(i*m.sourceStep);
        const float src=LayerExtraction::monoSample(source,srcIndex);

        const float crack=src*attackEnv*m.sourceAmount
                         *(0.55f+0.75f*p.punch)*(0.72f+0.38f*p.snap);
        const float bodyEnv=envExp(t,0.045f+0.22f*p.size+0.10f*p.body);
        const float body=std::sin(2.0f*pi*m.bodyFrequencyHz*t
                                 +0.22f*std::sin(2.0f*pi*m.bodyFrequencyHz*1.8f*t))
                        *bodyEnv*m.reinforcementAmount*(0.18f+0.42f*p.body);
        const float noiseEnv=envExp(t,m.tailTimeConstant);
        const float texture=(0.55f*rng.bipolar()+0.45f*src)*noiseEnv
                            *(0.08f+0.34f*p.texture)*(0.55f+0.45f*p.noise);
        const float fallback=m.requiresFallback
            ? rng.bipolar()*attackEnv*m.reinforcementAmount*0.08f : 0.0f;
        const float opposite=p.crossBlend*0.10f*rng.bipolar()*envExp(t,0.08f);

        float y=(crack+body+texture+fallback+opposite)*(0.70f+0.55f*p.punch);
        y=std::tanh(y*m.drive)/std::tanh(std::max(1.0f,m.drive));
        const float panSpread=0.10f+0.55f*p.width;
        output.setSample(0,i,y*(1.0f-panSpread*0.10f));
        output.setSample(1,i,y*(1.0f+panSpread*0.10f));
    }
}
