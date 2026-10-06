#include "PresetManager.h"

bool PresetManager::save(const juce::File& file,const SnairParameters& p,juce::String& error)
{
    error.clear();
    auto* o=new juce::DynamicObject();
    o->setProperty("version",1); o->setProperty("mode",p.mode==SnairMode::clap?"clap":"snare");
    o->setProperty("source_character",p.sourceCharacter); o->setProperty("punch",p.punch); o->setProperty("snap",p.snap);
    o->setProperty("body",p.body); o->setProperty("texture",p.texture); o->setProperty("dirt",p.dirt); o->setProperty("size",p.size);
    o->setProperty("body_freq_hz",p.bodyFreqHz); o->setProperty("attack",p.attack); o->setProperty("noise",p.noise);
    o->setProperty("tail_ms",p.tailMs); o->setProperty("pitch_st",p.pitchSt); o->setProperty("tone",p.tone); o->setProperty("drive_db",p.driveDb);
    o->setProperty("width",p.width); o->setProperty("clap_count",p.clapCount); o->setProperty("clap_spread_ms",p.clapSpreadMs);
    o->setProperty("cross_blend",p.crossBlend); o->setProperty("output_trim_db",p.outputTrimDb); o->setProperty("normalize_render",p.normalizeRender);
    o->setProperty("seed",static_cast<int64>(p.seed));
    if(!file.replaceWithText(juce::JSON::toString(juce::var(o),true))){ error="Could not save preset."; return false; }
    return true;
}

bool PresetManager::load(const juce::File& file,SnairParameters& p,juce::String& error)
{
    error.clear(); auto v=juce::JSON::parse(file);
    if(!v.isObject()){ error="Preset is not valid JSON."; return false; }
    auto* o=v.getDynamicObject(); if(o==nullptr){error="Preset object is invalid.";return false;}
    SnairParameters q=p;
    auto f=[&](const char* n,float& x){ if(o->hasProperty(n)) x=static_cast<float>(o->getProperty(n)); };
    f("source_character",q.sourceCharacter);f("punch",q.punch);f("snap",q.snap);f("body",q.body);f("texture",q.texture);f("dirt",q.dirt);f("size",q.size);
    f("body_freq_hz",q.bodyFreqHz);f("attack",q.attack);f("noise",q.noise);f("tail_ms",q.tailMs);f("pitch_st",q.pitchSt);f("tone",q.tone);f("drive_db",q.driveDb);
    f("width",q.width);f("clap_spread_ms",q.clapSpreadMs);f("cross_blend",q.crossBlend);f("output_trim_db",q.outputTrimDb);
    if(o->hasProperty("mode")) q.mode=o->getProperty("mode").toString()=="clap"?SnairMode::clap:SnairMode::snare;
    if(o->hasProperty("clap_count")) q.clapCount=static_cast<int>(o->getProperty("clap_count"));
    if(o->hasProperty("normalize_render")) q.normalizeRender=static_cast<bool>(o->getProperty("normalize_render"));
    if(o->hasProperty("seed")) q.seed=static_cast<uint32_t>(static_cast<int64>(o->getProperty("seed")));
    q.sanitize(); p=q; return true;
}
