#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

enum class SnairMode { snare = 0, clap = 1 };

struct SnairParameters
{
    SnairMode mode = SnairMode::snare;
    float sourceCharacter = 0.5f;
    float punch = 0.5f;
    float snap = 0.5f;
    float body = 0.5f;
    float texture = 0.5f;
    float dirt = 0.2f;
    float size = 0.5f;
    float bodyFreqHz = 190.0f;
    float attack = 0.5f;
    float noise = 0.5f;
    float tailMs = 300.0f;
    float pitchSt = 0.0f;
    float tone = 0.0f;
    float driveDb = 3.0f;
    float width = 0.5f;
    int clapCount = 4;
    float clapSpreadMs = 18.0f;
    float crossBlend = 0.0f;
    float outputTrimDb = 0.0f;
    bool normalizeRender = true;
    uint32_t seed = 1u;

    void sanitize() noexcept
    {
        auto c=[](float v,float lo,float hi,float d){ return std::isfinite(v)?std::clamp(v,lo,hi):d; };
        sourceCharacter=c(sourceCharacter,0,1,0.5f); punch=c(punch,0,1,0.5f); snap=c(snap,0,1,0.5f);
        body=c(body,0,1,0.5f); texture=c(texture,0,1,0.5f); dirt=c(dirt,0,1,0.2f); size=c(size,0,1,0.5f);
        bodyFreqHz=c(bodyFreqHz,70,450,190); attack=c(attack,0,1,0.5f); noise=c(noise,0,1,0.5f);
        tailMs=c(tailMs,20,2000,300); pitchSt=c(pitchSt,-24,24,0); tone=c(tone,-1,1,0); driveDb=c(driveDb,0,24,3);
        width=c(width,0,1,0.5f); clapCount=std::clamp(clapCount,2,6); clapSpreadMs=c(clapSpreadMs,8,35,18);
        crossBlend=c(crossBlend,0,1,0); outputTrimDb=c(outputTrimDb,-24,12,0);
    }

    bool operator==(const SnairParameters& o) const noexcept
    {
        return mode==o.mode && sourceCharacter==o.sourceCharacter && punch==o.punch && snap==o.snap &&
               body==o.body && texture==o.texture && dirt==o.dirt && size==o.size && bodyFreqHz==o.bodyFreqHz &&
               attack==o.attack && noise==o.noise && tailMs==o.tailMs && pitchSt==o.pitchSt && tone==o.tone &&
               driveDb==o.driveDb && width==o.width && clapCount==o.clapCount && clapSpreadMs==o.clapSpreadMs &&
               crossBlend==o.crossBlend && outputTrimDb==o.outputTrimDb && normalizeRender==o.normalizeRender &&
               seed==o.seed;
    }
};
