#include "FactoryPresets.h"

namespace
{
SnairParameters snare(float punch,float snap,float body,float texture,float dirt,float size,
                      float freq,float tone,float width,uint32_t seed)
{
    SnairParameters p;
    p.mode=SnairMode::snare;
    p.punch=punch;p.snap=snap;p.body=body;p.texture=texture;p.dirt=dirt;p.size=size;
    p.bodyFreqHz=freq;p.tone=tone;p.width=width;p.seed=seed;
    p.tailMs=120.0f+size*520.0f;
    p.sourceCharacter=0.48f;
    p.sanitize();
    return p;
}

SnairParameters clap(float punch,float snap,float texture,float dirt,float size,
                     int count,float spread,float tone,float width,uint32_t seed)
{
    SnairParameters p;
    p.mode=SnairMode::clap;
    p.punch=punch;p.snap=snap;p.texture=texture;p.dirt=dirt;p.size=size;
    p.clapCount=count;p.clapSpreadMs=spread;p.tone=tone;p.width=width;p.seed=seed;
    p.noise=0.68f;p.sourceCharacter=0.42f;p.tailMs=100.0f+size*500.0f;
    p.sanitize();
    return p;
}
}

const std::vector<FactoryPreset>& FactoryPresets::all()
{
    static const std::vector<FactoryPreset> presets={
        {"Tight Studio Snare",snare(0.78f,0.64f,0.48f,0.26f,0.10f,0.22f,205.0f,0.12f,0.28f,1101)},
        {"Deep Wood Snare",snare(0.62f,0.48f,0.88f,0.34f,0.18f,0.70f,138.0f,-0.22f,0.42f,1102)},
        {"Bright Electronic Snare",snare(0.70f,0.90f,0.38f,0.58f,0.32f,0.36f,235.0f,0.58f,0.72f,1103)},
        {"Broken Machine Snare",snare(0.86f,0.74f,0.52f,0.82f,0.78f,0.46f,178.0f,0.34f,0.84f,1104)},
        {"Gated 80s Snare",[]{auto p=snare(0.82f,0.70f,0.62f,0.48f,0.22f,0.58f,196.0f,0.20f,0.70f,1105);p.room=0.65f;return p;}()},
        {"Dry Hand Clap",clap(0.55f,0.82f,0.38f,0.08f,0.18f,3,11.0f,0.18f,0.48f,2101)},
        {"Wide Club Clap",clap(0.66f,0.88f,0.64f,0.24f,0.48f,5,18.0f,0.40f,0.92f,2102)},
        {"Loose Layered Clap",clap(0.48f,0.72f,0.76f,0.34f,0.64f,6,29.0f,-0.10f,0.78f,2103)},
        {"Crushed Digital Clap",clap(0.74f,0.92f,0.72f,0.86f,0.30f,4,14.0f,0.62f,0.66f,2104)},
        {"Arena Gated Clap",[]{auto p=clap(0.70f,0.80f,0.60f,0.20f,0.62f,5,20.0f,0.25f,0.85f,2105);p.room=0.55f;return p;}()}
    };
    return presets;
}
