#include <JuceHeader.h>
#include "FactoryPresets.h"
#include "HitPlayer.h"
#include "LayerExtraction.h"
#include "PluginProcessor.h"
#include "PresetManager.h"
#include "SnairEngine.h"
#include "SourceAnalyzer.h"
#include "SourceLoader.h"
#include "WavExporter.h"

namespace
{
constexpr double sr = 48000.0;

SourceAudio makeSource(int kind, int frames = 48000, int channels = 1)
{
    SourceAudio s;
    s.sampleRate = sr; s.channelCount = channels; s.frameCount = frames;
    s.samples.setSize(channels, frames);
    s.samples.clear();
    juce::Random rnd(7);
    for (int i = 0; i < frames; ++i)
    {
        const float t = static_cast<float>(i / sr);
        float x = 0.0f;
        switch (kind)
        {
            case 0: x = 0.35f * std::sin(juce::MathConstants<float>::twoPi * 180.0f * t) * std::exp(-t * 6.0f);
                    if (i >= 30000 && i < 30300) x += (1.0f - (i - 30000) / 300.0f) * 0.8f; break; // late transient
            case 1: x = i == frames / 3 ? 1.0f : 0.0f; break;                                    // impulse
            case 2: x = 0.5f * std::sin(juce::MathConstants<float>::twoPi * 440.0f * t); break; // sustained tone
            case 3: x = 0.4f * (rnd.nextFloat() * 2.0f - 1.0f); break;                          // noise
            default: break;                                                                      // silence
        }
        for (int c = 0; c < channels; ++c) s.samples.setSample(c, i, x * (c == 0 ? 1.0f : 0.7f));
    }
    return s;
}

bool allFinite(const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (!std::isfinite(b.getSample(c, i))) return false;
    return true;
}

float diff(const RenderedHit& a, const RenderedHit& b)
{
    float d = 0.0f;
    const int n = std::min(a.samples.getNumSamples(), b.samples.getNumSamples());
    for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) d += std::abs(a.samples.getSample(c, i) - b.samples.getSample(c, i));
    return d + std::abs(a.samples.getNumSamples() - b.samples.getNumSamples());
}

bool writeWav(const juce::File& f, const juce::AudioBuffer<float>& b, double rate, bool aiff = false)
{
    f.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = f.createOutputStream();
    if (!stream) return false;
    auto opts = juce::AudioFormatWriter::Options {}.withSampleRate(rate).withNumChannels(b.getNumChannels()).withBitsPerSample(24);
    std::unique_ptr<juce::AudioFormatWriter> w;
    if (aiff) { juce::AiffAudioFormat fmt; w = fmt.createWriterFor(stream, opts); }
    else      { juce::WavAudioFormat fmt;  w = fmt.createWriterFor(stream, opts); }
    return w && w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
}
}

class AnalyzerTests final : public juce::UnitTest
{
public:
    AnalyzerTests() : juce::UnitTest("SourceAnalyzer") {}
    void runTest() override
    {
        SourceAnalysis a; juce::String e;
        beginTest("late transient in a long source is found");
        auto s = makeSource(0);
        expect(SourceAnalyzer::analyze(s, a, e));
        expect(std::abs(static_cast<int>(a.strongestTransientSample) - 30000) < 800, "transient at " + juce::String(a.strongestTransientSample));
        expect(a.fingerprint != 0 && a.rms > 0.0f && a.crest > 1.0f);
        expect(a.dominantBodyHz >= 70.0f && a.dominantBodyHz <= 450.0f);
        beginTest("impulse, tone and noise descriptors");
        auto imp = makeSource(1); expect(SourceAnalyzer::analyze(imp, a, e)); expect(a.crest > 20.0f);
        auto tone = makeSource(2); expect(SourceAnalyzer::analyze(tone, a, e)); const float tonalNoise = a.noisyEstimate;
        auto nz = makeSource(3); expect(SourceAnalyzer::analyze(nz, a, e)); expect(a.noisyEstimate > tonalNoise);
        beginTest("silence is rejected consistently");
        auto sil = makeSource(4); expect(!SourceAnalyzer::analyze(sil, a, e)); expect(e.isNotEmpty());
        beginTest("very short buffers do not read out of range");
        auto tiny = makeSource(1, 3); expect(SourceAnalyzer::analyze(tiny, a, e) || e.isNotEmpty());
    }
};

class EngineTests final : public juce::UnitTest
{
public:
    EngineTests() : juce::UnitTest("SnairEngine") {}
    void runTest() override
    {
        juce::String e;
        for (int kind : { 0, 1, 2, 3 })
            for (int mode : { 0, 1 })
            {
                beginTest("finite, peak-safe render: source " + juce::String(kind) + (mode ? " clap" : " snare"));
                auto s = makeSource(kind, 48000, 2); SourceAnalysis a; expect(SourceAnalyzer::analyze(s, a, e));
                SnairParameters p; p.mode = mode ? SnairMode::clap : SnairMode::snare; p.seed = 42;
                auto h = SnairEngine::render(s, a, p, 1, e, sr);
                expect(h != nullptr, e);
                if (!h) continue;
                expect(allFinite(h->samples));
                expect(h->peak <= 1.0f && h->peak > 0.5f, "peak " + juce::String(h->peak));
                expectEquals(h->samples.getNumChannels(), 2);
                expect(h->samples.getNumSamples() <= static_cast<int>(SnairEngine::maxHitSeconds * sr) + 1);
                // Attack: early energy dominates the end of the hit.
                expect(h->samples.getMagnitude(0, h->samples.getNumSamples() / 10) > 4.0f * h->samples.getMagnitude(h->samples.getNumSamples() * 9 / 10, h->samples.getNumSamples() / 10));
            }

        auto s = makeSource(0, 48000, 2); SourceAnalysis a; expect(SourceAnalyzer::analyze(s, a, e));
        SnairParameters p; p.seed = 42;
        beginTest("determinism and seed sensitivity");
        auto h1 = SnairEngine::render(s, a, p, 1, e, sr), h2 = SnairEngine::render(s, a, p, 2, e, sr);
        expect(h1 && h2 && diff(*h1, *h2) < 1.0e-6f);
        p.seed = 43; auto h3 = SnairEngine::render(s, a, p, 3, e, sr);
        expect(h3 && diff(*h1, *h3) > 0.1f);

        beginTest("every macro and room materially changes the render");
        p.seed = 42;
        float* fields[] = { &p.sourceCharacter, &p.punch, &p.snap, &p.body, &p.texture, &p.dirt, &p.size, &p.room, &p.noise, &p.attack, &p.width, &p.crossBlend };
        for (auto* f : fields)
        {
            const float old = *f;
            *f = old > 0.5f ? 0.0f : 1.0f;
            auto h = SnairEngine::render(s, a, p, 4, e, sr);
            expect(h && diff(*h1, *h) > 0.05f);
            *f = old;
        }
        beginTest("Source Character changes multiple stages, not a crossfade");
        p.sourceCharacter = 0.0f; auto lo = SnairEngine::render(s, a, p, 5, e, sr);
        p.sourceCharacter = 1.0f; auto hi = SnairEngine::render(s, a, p, 6, e, sr);
        expect(lo && hi && diff(*lo, *hi) > 1.0f);
        p.sourceCharacter = 0.5f;

        beginTest("clap burst count/spacing bounds and deterministic jitter");
        p.mode = SnairMode::clap;
        for (int count = 2; count <= 6; ++count)
        {
            p.clapCount = count; p.clapSpreadMs = 35.0f;
            DeterministicRng rng(9);
            const int frames = SnairEngine::framesFor(p, sr);
            auto m = LayerExtraction::prepare(s, a, p, sr, frames, rng);
            juce::AudioBuffer<float> b(2, frames); b.clear();
            std::vector<int> starts;
            snairdsp::addBursts(b.getWritePointer(0), b.getWritePointer(1), m, p, sr, count, p.clapSpreadMs, 1.0f, rng, &starts);
            expectEquals(static_cast<int>(starts.size()), count);
            for (size_t i = 1; i < starts.size(); ++i)
            {
                const double ms = (starts[i] - starts[i - 1]) * 1000.0 / sr;
                expect(ms >= 7.99 && ms <= 35.01, "spacing " + juce::String(ms));
            }
        }
        auto c1 = SnairEngine::render(s, a, p, 7, e, sr), c2 = SnairEngine::render(s, a, p, 8, e, sr);
        expect(c1 && c2 && diff(*c1, *c2) < 1.0e-6f);

        beginTest("render at another sample rate and with non-normalized output");
        p.normalizeRender = false; p.driveDb = 24.0f; p.dirt = 1.0f;
        auto loud = SnairEngine::render(s, a, p, 9, e, 44100.0);
        expect(loud && loud->peak <= 0.981f && std::abs(loud->sampleRate - 44100.0) < 0.01);

        beginTest("Randomize and Mutate stay bounded and preserve mode");
        SnairParameters base; base.mode = SnairMode::clap; base.sanitize();
        float randomMove = 0.0f, mutateMove = 0.0f;
        for (uint32_t k = 1; k < 200; ++k)
        {
            auto r = SnairEngine::randomized(base, k * 77u), m = SnairEngine::mutated(base, k * 91u);
            for (auto* q : { &r, &m })
            {
                auto c = *q; c.sanitize();
                expect(c.renderHash() == q->renderHash(), "out of range value");
                expect(q->mode == base.mode && q->seed <= maxGeneratedSeed);
            }
            randomMove += std::abs(r.punch - base.punch) + std::abs(r.texture - base.texture) + std::abs(r.size - base.size);
            const float mm = std::abs(m.punch - base.punch) + std::abs(m.texture - base.texture) + std::abs(m.size - base.size);
            expect(mm <= 3 * 0.1501f);
            mutateMove += mm;
        }
        expect(mutateMove < randomMove * 0.6f);

        beginTest("difficult sources: one-sample buffer");
        auto tiny = makeSource(1, 1); SourceAnalysis ta; ta.fingerprint = 1; ta.crest = 1.0f;
        auto th = SnairEngine::render(tiny, ta, SnairParameters {}, 1, e, sr);
        expect(th == nullptr && e.isNotEmpty());
    }
};

class PlaybackTests final : public juce::UnitTest
{
public:
    PlaybackTests() : juce::UnitTest("HitPlayer") {}
    void runTest() override
    {
        RenderedHit hit; hit.sampleRate = sr; hit.samples.setSize(2, 4800);
        for (int i = 0; i < 4800; ++i) { hit.samples.setSample(0, i, 0.5f); hit.samples.setSample(1, i, 0.5f); }
        HitPlayer player; player.prepare(sr); player.setHit(&hit);
        juce::AudioBuffer<float> out(2, 256);
        beginTest("note starts a voice; velocity scales amplitude");
        player.noteOn(1.0f, 36, false); out.clear(); player.render(out, 0, 256, 1, 1);
        expectWithinAbsoluteError(out.getSample(0, 10), 0.5f, 1.0e-5f);
        player.allNotesOff(); player.noteOn(0.5f, 36, false); out.clear(); player.render(out, 0, 256, 1, 1);
        expectWithinAbsoluteError(out.getSample(0, 10), 0.25f, 1.0e-5f);
        beginTest("overlap and 16-voice cap");
        player.allNotesOff();
        for (int i = 0; i < 20; ++i) player.noteOn(1.0f, 38, false);
        expectEquals(player.activeVoiceCount(), 16);
        beginTest("one-shot plays to the end (note-off does not cut) then frees");
        player.allNotesOff(); player.noteOn(1.0f, 36, false);
        for (int b = 0; b < 4800 / 256 + 2; ++b) { out.clear(); player.render(out, 0, 256, 1, 1); }
        expectEquals(player.activeVoiceCount(), 0);
        beginTest("key tracking doubles playback rate an octave up");
        player.noteOn(1.0f, 72, true);
        for (int b = 0; b < 4800 / 512 + 1; ++b) { out.clear(); player.render(out, 0, 256, 1, 1); }
        expectEquals(player.activeVoiceCount(), 0);
        beginTest("no hit outputs silence");
        player.setHit(nullptr); player.noteOn(1.0f, 36, false); out.clear(); player.render(out, 0, 256, 1, 1);
        expectEquals(out.getMagnitude(0, 256), 0.0f);
    }
};

class IoTests final : public juce::UnitTest
{
public:
    IoTests() : juce::UnitTest("IO") {}
    void runTest() override
    {
        auto root = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("SnairCreatorTests_" + juce::String(juce::Random::getSystemRandom().nextInt()));
        root.createDirectory();
        juce::String e;
        auto s = makeSource(0, 24000, 2);
        beginTest("WAV and AIFF decode; corrupt and disguised files rejected");
        expect(writeWav(root.getChildFile("a.wav"), s.samples, sr));
        expect(writeWav(root.getChildFile("a.aiff"), s.samples, sr, true));
        auto w = SourceLoader::decode(root.getChildFile("a.wav"), e); expect(w && w->channelCount == 2 && w->frameCount == 24000);
        auto ai = SourceLoader::decode(root.getChildFile("a.aiff"), e); expect(ai != nullptr);
        root.getChildFile("bad.wav").replaceWithText("not audio");
        expect(SourceLoader::decode(root.getChildFile("bad.wav"), e) == nullptr && e.isNotEmpty());
        expect(writeWav(root.getChildFile("x.mp3"), s.samples, sr));
        expect(SourceLoader::decode(root.getChildFile("x.mp3"), e) == nullptr);

        SourceAnalysis a; expect(SourceAnalyzer::analyze(*w, a, e));
        auto hit = SnairEngine::render(*w, a, SnairParameters {}, 1, e, sr);
        expect(hit != nullptr);
        beginTest("WAV export formats, rates and channels reopen correctly");
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        for (int bits : { 16, 24, 32 })
            for (int ch : { 1, 2 })
                for (double rate : { 0.0, 44100.0, 96000.0 })
                {
                    WavExportOptions o; o.bitDepth = bits; o.channels = ch; o.sampleRate = rate;
                    auto f = root.getChildFile("out_" + juce::String(bits) + "_" + juce::String(ch) + "_" + juce::String(static_cast<int>(rate)) + ".wav");
                    expect(WavExporter::write(f, *hit, o, e), e);
                    std::unique_ptr<juce::AudioFormatReader> r(fm.createReaderFor(f));
                    expect(r != nullptr);
                    if (!r) continue;
                    const double outRate = rate > 0 ? rate : sr;
                    expectEquals(static_cast<int>(r->bitsPerSample), bits);
                    expectEquals(static_cast<int>(r->numChannels), ch);
                    expectWithinAbsoluteError(r->sampleRate, outRate, 0.5);
                    expect(std::abs(r->lengthInSamples - static_cast<juce::int64>(hit->samples.getNumSamples() * outRate / sr)) <= 2);
                    if (rate == 0.0 && ch == 2)
                    {
                        juce::AudioBuffer<float> back(2, static_cast<int>(r->lengthInSamples));
                        r->read(&back, 0, back.getNumSamples(), 0, true, true);
                        float maxErr = 0.0f;
                        for (int i = 0; i < back.getNumSamples(); ++i) maxErr = std::max(maxErr, std::abs(back.getSample(0, i) - hit->samples.getSample(0, i)));
                        expect(maxErr < (bits == 16 ? 1.0e-4f : 1.0e-6f), "export mismatch " + juce::String(maxErr));
                    }
                }
        beginTest("invalid export settings fail without touching the destination");
        auto keep = root.getChildFile("keep.wav"); keep.replaceWithText("existing");
        WavExportOptions bad; bad.sampleRate = 1.0;
        expect(!WavExporter::write(keep, *hit, bad, e)); expectEquals(keep.loadFileAsString(), juce::String("existing"));

        beginTest("preset round trip and invalid field handling");
        SnairParameters p; p.mode = SnairMode::clap; p.punch = 0.9f; p.seed = 4242; p.room = 0.3f; p.keyTrack = true; p.clapCount = 6;
        auto pf = root.getChildFile("p.snairpreset");
        expect(PresetManager::save(pf, p, "/tmp/src.wav", 7, e));
        SnairParameters q; juce::String warn, src;
        expect(PresetManager::load(pf, q, e, warn, src));
        expect(q.renderHash() == p.renderHash() && q.keyTrack && warn.isEmpty() && src == "/tmp/src.wav");
        pf.replaceWithText("{\"version\":1,\"punch\":7,\"mode\":\"clap\"}");
        expect(PresetManager::load(pf, q, e, warn, src)); expect(warn.contains("punch") && q.mode == SnairMode::clap && q.punch == 0.5f);
        pf.replaceWithText("{nope");
        expect(!PresetManager::load(pf, q, e, warn, src));
        expect(FactoryPresets::all().size() >= 8);
        root.deleteRecursively();
    }
};

class ProcessorTests final : public juce::UnitTest
{
public:
    ProcessorTests() : juce::UnitTest("Processor") {}

    void pump(SnairCreatorProcessor& proc, int ms, std::function<bool()> done)
    {
        const auto end = juce::Time::getMillisecondCounter() + static_cast<juce::uint32>(ms);
        juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi;
        while (juce::Time::getMillisecondCounter() < end && !done())
        {
            juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
            proc.processBlock(buf, midi);
        }
    }

    void runTest() override
    {
        auto root = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("SnairCreatorProcTests");
        root.createDirectory();
        auto srcFile = root.getChildFile("source.wav");
        expect(writeWav(srcFile, makeSource(0, 24000, 2).samples, sr));

        beginTest("stable parameter IDs");
        SnairCreatorProcessor proc;
        for (auto* id : { "mode", "source_character", "punch", "snap", "body", "texture", "dirt", "size", "body_freq_hz", "attack", "noise",
                          "tail_ms", "pitch_st", "tone", "drive_db", "width", "clap_count", "clap_spread_ms", "cross_blend",
                          "output_trim_db", "normalize_render", "seed", "room", "key_track" })
            expect(proc.state().getParameter(id) != nullptr, id);

        beginTest("load -> render -> preview/MIDI");
        proc.setPlayConfigDetails(0, 2, sr, 512);
        proc.prepareToPlay(sr, 512);
        { juce::AudioBuffer<float> empty(2, 0); juce::MidiBuffer none; proc.processBlock(empty, none); } // zero-length block is safe
        proc.loadSource(srcFile);
        pump(proc, 10000, [&] { return proc.currentHit() != nullptr && proc.getStatus() == SnairCreatorProcessor::Status::ready; });
        expect(proc.currentHit() != nullptr, proc.getStatusText());
        juce::AudioBuffer<float> buf(2, 512); juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 100);
        proc.processBlock(buf, midi);
        expect(buf.getMagnitude(0, 100) == 0.0f && buf.getMagnitude(100, 412) > 0.01f, "sample-accurate note-on");
        expectEquals(static_cast<int>(proc.midiActivityCount()), 1);

        beginTest("parameter change re-renders off-thread and keeps old hit until swap");
        const auto first = proc.currentHit();
        auto p = proc.currentParams(); p.punch = 0.95f; proc.applyParameters(p);
        expect(proc.currentHit() == first);
        pump(proc, 10000, [&] { return proc.currentHit() != first && !proc.isHitStale(); });
        expect(proc.currentHit() != first && !proc.isHitStale());

        beginTest("rapid parameter changes coalesce to the newest render");
        for (int i = 0; i < 50; ++i) { p.texture = i / 50.0f; proc.applyParameters(p); juce::MessageManager::getInstance()->runDispatchLoopUntil(1); }
        pump(proc, 10000, [&] { return !proc.isHitStale() && proc.getStatus() == SnairCreatorProcessor::Status::ready; });
        expect(!proc.isHitStale());

        beginTest("randomize/mutate/undo restore the previous hit instantly");
        const auto beforeRandom = proc.currentHit();
        const auto beforeParams = proc.currentParams();
        proc.randomize();
        pump(proc, 10000, [&] { return proc.currentHit() != beforeRandom && !proc.isHitStale(); });
        expect(proc.currentParams().mode == beforeParams.mode);
        proc.mutate();
        pump(proc, 10000, [&] { return !proc.isHitStale(); });
        expect(proc.undo()); expect(proc.undo());
        expect(proc.currentHit() == beforeRandom && proc.currentParams().renderHash() == beforeParams.renderHash());

        beginTest("state round trip restores parameters and exact hit, with source missing");
        juce::MemoryBlock state;
        proc.getStateInformation(state);
        const auto saved = proc.currentHit();
        srcFile.deleteFile();
        SnairCreatorProcessor restored;
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        expect(restored.currentParams().renderHash() == proc.currentParams().renderHash());
        expect(restored.currentHit() != nullptr && diff(*restored.currentHit(), *saved) < 1.0e-6f);
        expect(restored.isSourceMissing());
        juce::String msg;
        expect(restored.exportHit(root.getChildFile("restored.wav"), msg), msg);

        beginTest("malformed and future state is ignored safely");
        const auto hash = restored.currentParams().renderHash();
        const char junk[] = "garbage-state";
        restored.setStateInformation(junk, sizeof(junk));
        juce::ValueTree future("SnairCreatorState"); future.setProperty("version", 99, nullptr);
        juce::MemoryOutputStream fm; future.writeToStream(fm);
        restored.setStateInformation(fm.getData(), static_cast<int>(fm.getDataSize()));
        expect(restored.currentParams().renderHash() == hash && restored.currentHit() != nullptr);

        beginTest("editor instantiates");
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        expect(ed != nullptr && ed->getWidth() == 1000 && ed->getHeight() == 700);
        // Optional visual check: SNAIR_SNAPSHOT_DIR=<dir> writes PNGs of the main states.
        const auto snapDir = juce::SystemStats::getEnvironmentVariable("SNAIR_SNAPSHOT_DIR", {});
        auto snap = [&](const juce::String& name)
        {
            if (snapDir.isEmpty()) return;
            juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
            juce::File out = juce::File(snapDir).getChildFile(name + ".png");
            out.deleteFile();
            juce::FileOutputStream os(out);
            juce::PNGImageFormat().writeImageToStream(ed->createComponentSnapshot(ed->getLocalBounds()), os);
        };
        auto click = [&](const juce::String& textOnButton)
        {
            for (auto* c : ed->getChildren())
                if (auto* b = dynamic_cast<juce::Button*>(c); b != nullptr && b->getButtonText() == textOnButton) b->triggerClick();
            juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        };
        snap("main");
        click("SHOW HIT"); snap("hit_view"); click("SHOW SOURCE");
        click("ADVANCED"); snap("advanced");
        ed->setSize(760, 520); snap("advanced_min");
        click("ADVANCED"); snap("main_min");
        click("HELP"); snap("help");
        ed.reset();
        proc.releaseResources();
        root.deleteRecursively();
    }
};

static AnalyzerTests analyzerTests;
static EngineTests engineTests;
static PlaybackTests playbackTests;
static IoTests ioTests;
static ProcessorTests processorTests;
