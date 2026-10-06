#pragma once
#include "Parameters.h"
#include "SourceAnalysis.h"
#include "SourceAudio.h"

struct LayerMaterial
{
    int transientSample=0;
    double sourceStep=1.0;
    float bodyFrequencyHz=190.0f;
    float sourceAmount=0.5f;
    float reinforcementAmount=0.5f;
    float drive=1.0f;
    float tailTimeConstant=0.1f;
    bool requiresFallback=false;
};

class LayerExtraction
{
public:
    static LayerMaterial prepare(const SourceAudio&,const SourceAnalysis&,const SnairParameters&,double outputSampleRate);
    static float monoSample(const SourceAudio&,int sampleIndex);
};
