#include "PresetManager.h"

juce::File PresetManager::userFolder()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Circuit Drift Labs").getChildFile("SnairCreator").getChildFile("Presets");
}

bool PresetManager::save(const juce::File& file, const SnairParameters& p, const juce::String& sourcePath,
                         uint64_t fingerprint, juce::String& error)
{
    error.clear();
    auto* o = new juce::DynamicObject();
    juce::var root(o);
    o->setProperty("format", "SnairCreatorPreset");
    o->setProperty("version", 1);
    o->setProperty("mode", p.mode == SnairMode::clap ? "clap" : "snare");
    o->setProperty("source_character", p.sourceCharacter); o->setProperty("punch", p.punch); o->setProperty("snap", p.snap);
    o->setProperty("body", p.body); o->setProperty("texture", p.texture); o->setProperty("dirt", p.dirt); o->setProperty("size", p.size);
    o->setProperty("body_freq_hz", p.bodyFreqHz); o->setProperty("attack", p.attack); o->setProperty("noise", p.noise);
    o->setProperty("tail_ms", p.tailMs); o->setProperty("pitch_st", p.pitchSt); o->setProperty("tone", p.tone); o->setProperty("drive_db", p.driveDb);
    o->setProperty("width", p.width); o->setProperty("clap_count", p.clapCount); o->setProperty("clap_spread_ms", p.clapSpreadMs);
    o->setProperty("cross_blend", p.crossBlend); o->setProperty("output_trim_db", p.outputTrimDb); o->setProperty("normalize_render", p.normalizeRender);
    o->setProperty("seed", static_cast<juce::int64>(p.seed)); o->setProperty("room", p.room); o->setProperty("key_track", p.keyTrack);
    o->setProperty("source", sourcePath.isEmpty() ? "none" : "external");
    o->setProperty("source_path", sourcePath);
    o->setProperty("source_fingerprint", juce::String::toHexString(static_cast<juce::int64>(fingerprint)));
    file.getParentDirectory().createDirectory();
    juce::TemporaryFile temp(file);
    if (!temp.getFile().replaceWithText(juce::JSON::toString(root, false)) || !temp.overwriteTargetFileWithTemporary())
    {
        error = "Could not write the preset to " + file.getFullPathName();
        return false;
    }
    return true;
}

bool PresetManager::load(const juce::File& file, SnairParameters& out, juce::String& error, juce::String& warning, juce::String& sourcePath)
{
    error.clear(); warning.clear(); sourcePath.clear();
    if (!file.existsAsFile() || file.getSize() > 256 * 1024) { error = "Preset file is missing or too large."; return false; }
    const auto v = juce::JSON::parse(file.loadFileAsString());
    auto* o = v.getDynamicObject();
    if (o == nullptr) { error = "Preset is not valid JSON."; return false; }
    if (o->hasProperty("version") && static_cast<int>(o->getProperty("version")) > 1) { error = "Preset was made by a newer SnairCreator."; return false; }

    SnairParameters q; // fields not present use documented defaults
    juce::StringArray bad;
    auto f = [&](const char* n, float& x, float lo, float hi)
    {
        if (!o->hasProperty(n)) return;
        const auto& pv = o->getProperty(n);
        const double d = static_cast<double>(pv);
        if ((pv.isDouble() || pv.isInt() || pv.isInt64()) && std::isfinite(d) && d >= lo && d <= hi) x = static_cast<float>(d);
        else bad.add(n);
    };
    f("source_character", q.sourceCharacter, 0, 1); f("punch", q.punch, 0, 1); f("snap", q.snap, 0, 1); f("body", q.body, 0, 1);
    f("texture", q.texture, 0, 1); f("dirt", q.dirt, 0, 1); f("size", q.size, 0, 1); f("body_freq_hz", q.bodyFreqHz, 70, 450);
    f("attack", q.attack, 0, 1); f("noise", q.noise, 0, 1); f("tail_ms", q.tailMs, 20, 2000); f("pitch_st", q.pitchSt, -24, 24);
    f("tone", q.tone, -1, 1); f("drive_db", q.driveDb, 0, 24); f("width", q.width, 0, 1); f("clap_spread_ms", q.clapSpreadMs, 8, 35);
    f("cross_blend", q.crossBlend, 0, 1); f("output_trim_db", q.outputTrimDb, -24, 12); f("room", q.room, 0, 1);
    float count = static_cast<float>(q.clapCount), seed = static_cast<float>(q.seed);
    f("clap_count", count, 2, 6); q.clapCount = juce::roundToInt(count);
    if (o->hasProperty("seed"))
    {
        const auto s = static_cast<juce::int64>(o->getProperty("seed"));
        if (s >= 0 && s <= 2147483647) q.seed = static_cast<uint32_t>(s); else bad.add("seed");
    }
    juce::ignoreUnused(seed);
    if (o->hasProperty("mode"))
    {
        const auto m = o->getProperty("mode").toString();
        if (m == "clap") q.mode = SnairMode::clap; else if (m == "snare") q.mode = SnairMode::snare; else bad.add("mode");
    }
    if (o->hasProperty("normalize_render")) q.normalizeRender = static_cast<bool>(o->getProperty("normalize_render"));
    if (o->hasProperty("key_track")) q.keyTrack = static_cast<bool>(o->getProperty("key_track"));
    sourcePath = o->getProperty("source_path").toString();
    if (!bad.isEmpty()) warning = "Invalid preset fields reset to defaults: " + bad.joinIntoString(", ");
    q.sanitize();
    out = q;
    return true;
}
