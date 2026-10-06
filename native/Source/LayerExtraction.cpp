#include "LayerExtraction.h"
#include <algorithm>
#include <cmath>

LayerMaterial LayerExtraction::prepare(const SourceAudio& source,
                                       const SourceAnalysis& analysis,
                                       const SnairParameters& p,
                                       double outputSampleRate)
{
    LayerMaterial m;
    m.transientSample=std::clamp(static_cast<int>(analysis.strongestTransientSample),0,
                                 std::max(0,source.samples.getNumSamples()-1));
    const float pitchRatio=std::pow(2.0f,p.pitchSt/12.0f);
    m.sourceStep=(source.sampleRate/outputSampleRate)*pitchRatio;
    m.bodyFrequencyHz=std::clamp(0.55f*p.bodyFreqHz+0.45f*analysis.dominantBodyHz,70.0f,450.0f);
    m.sourceAmount=0.10f+0.75f*p.sourceCharacter;
    m.reinforcementAmount=0.85f-0.65f*p.sourceCharacter;
    m.drive=1.0f+3.0f*p.dirt+0.07f*p.driveDb;
    const float requestedTail=std::clamp(p.tailMs/1000.0f,0.02f,2.0f);
    m.tailTimeConstant=std::clamp(requestedTail*(0.34f+0.40f*p.size),0.015f,1.2f);
    m.requiresFallback=analysis.crest<1.35f || analysis.transientCandidates.empty();
    return m;
}

float LayerExtraction::monoSample(const SourceAudio& source,int sampleIndex)
{
    if(source.samples.getNumSamples()==0 || source.samples.getNumChannels()==0) return 0.0f;
    const int i=std::clamp(sampleIndex,0,source.samples.getNumSamples()-1);
    float value=0.0f;
    for(int ch=0;ch<source.samples.getNumChannels();++ch)
        value+=source.samples.getSample(ch,i);
    return value/static_cast<float>(source.samples.getNumChannels());
}
