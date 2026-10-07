#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SnairEngine.h"
#include "SourceAnalyzer.h"
#include "SourceLoader.h"
#include "DeterministicRng.h"

namespace
{
constexpr size_t maxUndo = 16;
constexpr int maxStateHitSamples = static_cast<int>(SnairEngine::maxHitSeconds * 192000.0) + 16;

juce::String sanitizeFilePart(juce::String s)
{
    s = juce::File::createLegalFileName(s).removeCharacters(" .,;'\"`!@#$%^&*()[]{}+=~");
    return s.isEmpty() ? juce::String("Source") : s.substring(0, 40);
}
}

// Background render thread. Coalesces requests: only the newest pending request is kept, and a
// running render is cancelled when a newer one arrives.
class SnairCreatorProcessor::Worker final : public juce::Thread
{
public:
    explicit Worker(SnairCreatorProcessor& p) : juce::Thread("SnairCreator render"), owner(p) { startThread(); }
    ~Worker() override { signalThreadShouldExit(); cancel.store(true); wake.signal(); stopThread(4000); }

    void submit(Request r)
    {
        { std::lock_guard<std::mutex> l(lock); pending = std::move(r); hasPending = true; cancel.store(true); }
        wake.signal();
    }

    void run() override
    {
        while (!threadShouldExit())
        {
            wake.wait(-1);
            while (!threadShouldExit())
            {
                wait(20); // debounce rapid knob movement
                Request r;
                {
                    std::lock_guard<std::mutex> l(lock);
                    if (!hasPending) break;
                    r = std::move(pending);
                    hasPending = false;
                    cancel.store(false);
                }
                Result res;
                res.id = r.id;
                if (r.source && r.analysis)
                    res.hit = SnairEngine::render(*r.source, *r.analysis, r.params, r.id, res.error, r.rate, &cancel);
                else
                    res.error = "No source is loaded.";
                if (cancel.load() && res.hit == nullptr) continue; // superseded
                {
                    std::lock_guard<std::mutex> l(owner.resultLock);
                    owner.completed = std::move(res);
                }
                owner.triggerAsyncUpdate();
            }
        }
    }

private:
    SnairCreatorProcessor& owner;
    std::mutex lock;
    Request pending;
    bool hasPending = false;
    std::atomic<bool> cancel { false };
    juce::WaitableEvent wake;
};

juce::AudioProcessorValueTreeState::ParameterLayout SnairCreatorProcessor::createLayout()
{
    using namespace juce;
    using F = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    // Percent display that round-trips: "39 %" (or "39" / "0.39") parses back to the same value.
    auto pct = A().withStringFromValueFunction([](float v, int) { return String(v * 100.0f, 1) + " %"; })
                  .withValueFromStringFunction([](const String& t)
                  {
                      const auto v = t.retainCharacters("0123456789.-").getFloatValue();
                      return juce::jlimit(0.0f, 1.0f, t.containsChar('%') || v > 1.0f ? v / 100.0f : v);
                  });
    auto unit01 = [&](const char* id, const char* name, float def)
    { p.push_back(std::make_unique<F>(ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def, pct)); };

    p.push_back(std::make_unique<AudioParameterChoice>(ParameterID { ParamIDs::mode, 1 }, "Mode", StringArray { "Snare", "Clap" }, 0));
    unit01(ParamIDs::sourceCharacter, "Source Character", 0.5f);
    unit01(ParamIDs::punch, "Punch", 0.5f);
    unit01(ParamIDs::snap, "Snap", 0.5f);
    unit01(ParamIDs::body, "Body", 0.5f);
    unit01(ParamIDs::texture, "Texture", 0.5f);
    unit01(ParamIDs::dirt, "Dirt", 0.2f);
    unit01(ParamIDs::size, "Size", 0.5f);
    p.push_back(std::make_unique<F>(ParameterID { ParamIDs::bodyFreqHz, 1 }, "Body Freq", NormalisableRange<float>(70.0f, 450.0f, 0.1f, 0.5f), 190.0f, A().withLabel("Hz")));
    unit01(ParamIDs::attack, "Attack", 0.5f);
    unit01(ParamIDs::noise, "Noise", 0.5f);
    p.push_back(std::make_unique<F>(ParameterID { ParamIDs::tailMs, 1 }, "Tail", NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.4f), 300.0f, A().withLabel("ms")));
    p.push_back(std::make_unique<F>(ParameterID { ParamIDs::pitchSt, 1 }, "Pitch", NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f, A().withLabel("st")));
    p.push_back(std::make_unique<F>(ParameterID { ParamIDs::tone, 1 }, "Tone", NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<F>(ParameterID { ParamIDs::driveDb, 1 }, "Drive", NormalisableRange<float>(0.0f, 24.0f, 0.1f), 3.0f, A().withLabel("dB")));
    unit01(ParamIDs::width, "Width", 0.5f);
    p.push_back(std::make_unique<AudioParameterInt>(ParameterID { ParamIDs::clapCount, 1 }, "Clap Count", 2, 6, 4));
    p.push_back(std::make_unique<F>(ParameterID { ParamIDs::clapSpreadMs, 1 }, "Clap Spread", NormalisableRange<float>(8.0f, 35.0f, 0.1f), 18.0f, A().withLabel("ms")));
    unit01(ParamIDs::crossBlend, "Cross Blend", 0.0f);
    p.push_back(std::make_unique<F>(ParameterID { ParamIDs::outputTrimDb, 1 }, "Output Trim", NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel("dB")));
    p.push_back(std::make_unique<AudioParameterBool>(ParameterID { ParamIDs::normalizeRender, 1 }, "Normalize", true));
    // Float-backed so the full 0..2147483647 range converts without int overflow. Values above 2^24
    // cannot be represented exactly by float host automation; generated seeds stay below 1,000,000.
    p.push_back(std::make_unique<F>(ParameterID { ParamIDs::seed, 1 }, "Seed", NormalisableRange<float>(0.0f, 2147483647.0f, 1.0f), 1.0f,
        A().withStringFromValueFunction([](float v, int) { return String(static_cast<juce::int64>(juce::jlimit(0.0, 2147483647.0, std::round(static_cast<double>(v))))); })
           .withValueFromStringFunction([](const String& t) { return static_cast<float>(juce::jlimit<juce::int64>(0, 2147483647, t.retainCharacters("0123456789").getLargeIntValue())); })));
    unit01(ParamIDs::room, "Gated Room", 0.0f);
    p.push_back(std::make_unique<AudioParameterBool>(ParameterID { ParamIDs::keyTrack, 1 }, "Key Track", false));
    return { p.begin(), p.end() };
}

SnairCreatorProcessor::SnairCreatorProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SnairCreatorParams", createLayout())
{
    for (auto* param : AudioProcessor::getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
            apvts.addParameterListener(ranged->getParameterID(), this);
    trimParam = apvts.getRawParameterValue(ParamIDs::outputTrimDb);
    keyTrackParam = apvts.getRawParameterValue(ParamIDs::keyTrack);
    worker = std::make_unique<Worker>(*this);
    startTimerHz(30);
}

SnairCreatorProcessor::~SnairCreatorProcessor()
{
    stopTimer();
    cancelPendingUpdate();
    jobs.removeAllJobs(true, 4000);
    worker.reset();
}

SnairParameters SnairCreatorProcessor::currentParams() const
{
    auto v = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    SnairParameters p;
    p.mode = v(ParamIDs::mode) >= 0.5f ? SnairMode::clap : SnairMode::snare;
    p.sourceCharacter = v(ParamIDs::sourceCharacter); p.punch = v(ParamIDs::punch); p.snap = v(ParamIDs::snap);
    p.body = v(ParamIDs::body); p.texture = v(ParamIDs::texture); p.dirt = v(ParamIDs::dirt); p.size = v(ParamIDs::size);
    p.bodyFreqHz = v(ParamIDs::bodyFreqHz); p.attack = v(ParamIDs::attack); p.noise = v(ParamIDs::noise);
    p.tailMs = v(ParamIDs::tailMs); p.pitchSt = v(ParamIDs::pitchSt); p.tone = v(ParamIDs::tone); p.driveDb = v(ParamIDs::driveDb);
    p.width = v(ParamIDs::width); p.clapCount = juce::roundToInt(v(ParamIDs::clapCount)); p.clapSpreadMs = v(ParamIDs::clapSpreadMs);
    p.crossBlend = v(ParamIDs::crossBlend); p.outputTrimDb = v(ParamIDs::outputTrimDb); p.normalizeRender = v(ParamIDs::normalizeRender) >= 0.5f;
    p.seed = static_cast<uint32_t>(juce::jmax(0.0f, std::round(v(ParamIDs::seed))));
    p.room = v(ParamIDs::room); p.keyTrack = v(ParamIDs::keyTrack) >= 0.5f;
    p.sanitize();
    return p;
}

void SnairCreatorProcessor::applyParameters(const SnairParameters& in)
{
    auto p = in;
    p.sanitize();
    auto set = [this](const char* id, float value)
    {
        if (auto* param = apvts.getParameter(id))
        {
            const float norm = param->convertTo0to1(value);
            if (std::abs(param->getValue() - norm) > 1.0e-7f)
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(norm);
                param->endChangeGesture();
            }
        }
    };
    set(ParamIDs::mode, p.mode == SnairMode::clap ? 1.0f : 0.0f);
    set(ParamIDs::sourceCharacter, p.sourceCharacter); set(ParamIDs::punch, p.punch); set(ParamIDs::snap, p.snap);
    set(ParamIDs::body, p.body); set(ParamIDs::texture, p.texture); set(ParamIDs::dirt, p.dirt); set(ParamIDs::size, p.size);
    set(ParamIDs::bodyFreqHz, p.bodyFreqHz); set(ParamIDs::attack, p.attack); set(ParamIDs::noise, p.noise);
    set(ParamIDs::tailMs, p.tailMs); set(ParamIDs::pitchSt, p.pitchSt); set(ParamIDs::tone, p.tone); set(ParamIDs::driveDb, p.driveDb);
    set(ParamIDs::width, p.width); set(ParamIDs::clapCount, static_cast<float>(p.clapCount)); set(ParamIDs::clapSpreadMs, p.clapSpreadMs);
    set(ParamIDs::crossBlend, p.crossBlend); set(ParamIDs::outputTrimDb, p.outputTrimDb); set(ParamIDs::normalizeRender, p.normalizeRender ? 1.0f : 0.0f);
    set(ParamIDs::seed, static_cast<float>(p.seed)); set(ParamIDs::room, p.room); set(ParamIDs::keyTrack, p.keyTrack ? 1.0f : 0.0f);
    paramsDirty.store(true);
}

//==============================================================================
void SnairCreatorProcessor::prepareToPlay(double sampleRate, int)
{
    hostRate.store(sampleRate > 0.0 ? sampleRate : 48000.0);
    player.prepare(hostRate.load());
    player.allNotesOff();
    lastTrimGain = juce::Decibels::decibelsToGain(trimParam->load());
    audioPrepared.store(true);
    paramsDirty.store(true); // re-render at the new host rate if needed
}

void SnairCreatorProcessor::releaseResources()
{
    player.allNotesOff();
    player.publishInUse(inUse);
    audioPrepared.store(false);
}

bool SnairCreatorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet().isDisabled()
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void SnairCreatorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int num = buffer.getNumSamples();
    const bool keyTrack = keyTrackParam->load() >= 0.5f;
    const float targetTrim = juce::Decibels::decibelsToGain(trimParam->load());
    const float g0 = lastTrimGain;
    auto gainAt = [&](int s) { return num > 0 ? g0 + (targetTrim - g0) * static_cast<float>(s) / static_cast<float>(num) : targetTrim; };

    for (int n = previewRequests.exchange(0); n > 0; --n)
        player.noteOn(0.85f, 60, false);

    int pos = 0;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        const int at = juce::jlimit(0, num, metadata.samplePosition);
        if (at > pos) { player.render(buffer, pos, at - pos, gainAt(pos), gainAt(at)); pos = at; }
        if (msg.isNoteOn())
        {
            player.noteOn(msg.getFloatVelocity(), msg.getNoteNumber(), keyTrack);
            midiActivity.fetch_add(1);
        }
        else if (msg.isAllSoundOff() || msg.isAllNotesOff())
            player.allNotesOff();
        // Note-off intentionally does not cut one-shots.
    }
    if (num > pos) player.render(buffer, pos, num - pos, gainAt(pos), gainAt(num));
    lastTrimGain = targetTrim;
    midi.clear();

    voicesPlaying.store(player.activeVoiceCount());
    player.publishInUse(inUse);
    blockCounter.fetch_add(1, std::memory_order_acq_rel);
}

//==============================================================================
void SnairCreatorProcessor::setStatus(Status s, const juce::String& text)
{
    status = s;
    statusText = text;
    sendChangeMessage();
}

bool SnairCreatorProcessor::isHitStale() const
{
    if (!hit) return false;
    return hit->paramHash != currentParams().renderHash();
}

void SnairCreatorProcessor::timerCallback()
{
    if (paramsDirty.exchange(false)) requestRenderIfNeeded();
    collectGarbage();
}

void SnairCreatorProcessor::requestRenderIfNeeded(bool force)
{
    sendChangeMessage(); // keep the editor in sync with host automation
    if (!source || !analysis) return;
    const auto p = currentParams();
    const auto h = p.renderHash();
    const double rate = hostRate.load();
    const bool hitMatches = hit && hit->paramHash == h && hit->sourceFingerprint == analysis->fingerprint
                         && std::abs(hit->sampleRate - rate) < 0.5;
    if (!force && (hitMatches || (h == requestedHash && std::abs(rate - requestedRate) < 0.5 && latestRequestId != 0)))
        return;
    requestedHash = h;
    requestedRate = rate;
    latestRequestId = nextRequestId++;
    worker->submit({ source, analysis, p, rate, latestRequestId });
    setStatus(Status::rendering, hit ? "Rendering... (previous hit stays playable)" : "Rendering...");
}

void SnairCreatorProcessor::handleAsyncUpdate()
{
    Result r;
    { std::lock_guard<std::mutex> l(resultLock); r = std::move(completed); completed = {}; }
    if (r.id == 0 || r.id != latestRequestId) return; // stale result: discard
    latestRequestId = 0;
    if (r.hit)
    {
        publish(r.hit);
        juce::String text = "Ready  -  " + juce::String(r.hit->samples.getNumSamples() / r.hit->sampleRate, 2) + " s, peak "
                          + juce::String(juce::Decibels::gainToDecibels(r.hit->peak), 1) + " dBFS, seed " + juce::String(r.hit->seed);
        if (r.hit->usedFallback) text << "  (procedural reinforcement used: weak source transient)";
        setStatus(Status::ready, text);
        if (paramsDirty.load()) requestRenderIfNeeded();
    }
    else
    {
        requestedHash = 0;
        setStatus(Status::error, "Render failed: " + r.error + (hit ? " Previous hit is still active and does not match the current settings." : ""));
    }
}

void SnairCreatorProcessor::publish(RenderedHitPtr h)
{
    if (hit) retired.push_back({ hit, blockCounter.load() });
    {
        std::lock_guard<std::mutex> l(stateLock);
        hit = std::move(h);
    }
    player.setHit(hit.get());
    sendChangeMessage();
}

void SnairCreatorProcessor::collectGarbage()
{
    // A retired hit may be freed once the audio thread has completed a full block after its retirement
    // (so any pointer it loaded is now in the in-use table) and no voice references it.
    const uint64_t now = blockCounter.load(std::memory_order_acquire);
    const bool audioIdle = !audioPrepared.load();
    retired.erase(std::remove_if(retired.begin(), retired.end(), [&](const Retired& r)
    {
        if (r.hit == hit) return true;
        if (!audioIdle && now < r.block + 2) return false;
        for (auto& slot : inUse) if (slot.load(std::memory_order_acquire) == r.hit.get()) return false;
        return true;
    }), retired.end());
}

//==============================================================================
void SnairCreatorProcessor::loadSource(const juce::File& file)
{
    const auto token = ++sourceLoadToken;
    setStatus(Status::loading, "Loading " + file.getFileName() + "...");
    juce::WeakReference<SnairCreatorProcessor> weak(this);
    jobs.addJob([weak, file, token]
    {
        juce::String error;
        auto decoded = SourceLoader::decode(file, error);
        auto result = std::make_shared<SourceAnalysis>();
        if (decoded && !SourceAnalyzer::analyze(*decoded, *result, error)) decoded.reset();
        juce::MessageManager::callAsync([weak, decoded, result, error, file, token]
        {
            auto* self = weak.get();
            if (self == nullptr || token != self->sourceLoadToken) return;
            if (!decoded)
            {
                self->setStatus(Status::error, file.getFileName() + ": " + error
                                + (self->source ? " The previous source is unchanged." : ""));
                return;
            }
            const bool keep = self->sourceMissing && self->savedSourceFingerprint == result->fingerprint;
            self->installSource(decoded, result, keep);
        });
    });
}

void SnairCreatorProcessor::installSource(SourceAudioPtr s, std::shared_ptr<const SourceAnalysis> a, bool keepRestoredHit)
{
    source = std::move(s);
    analysis = std::move(a);
    {
        std::lock_guard<std::mutex> l(stateLock);
        sourceFile = source->sourceFile;
        sourceName = sourceFile.getFileNameWithoutExtension();
        savedSourceFingerprint = analysis->fingerprint;
    }
    sourceMissing = false;
    juce::ignoreUnused(keepRestoredHit);
    latestRequestId = 0;
    requestedHash = 0;
    setStatus(Status::ready, "Source loaded" + juce::String(source->sanitizedSampleCount > 0
                ? " (" + juce::String(source->sanitizedSampleCount) + " non-finite samples replaced)" : juce::String()));
    requestRenderIfNeeded(); // no-op when a restored hit already matches this source + settings
}

//==============================================================================
void SnairCreatorProcessor::pushUndo()
{
    undoHistory.push_back({ currentParams(), hit });
    while (undoHistory.size() > maxUndo) undoHistory.pop_front();
}

uint32_t SnairCreatorProcessor::nextVariationSeed()
{
    const auto p = currentParams();
    DeterministicRng rng((static_cast<uint64_t>(p.seed) << 20) ^ variationCounter++);
    return static_cast<uint32_t>(rng.next() & 0x7fffffff);
}

void SnairCreatorProcessor::randomize()
{
    pushUndo();
    applyParameters(SnairEngine::randomized(currentParams(), nextVariationSeed()));
}

void SnairCreatorProcessor::mutate()
{
    pushUndo();
    applyParameters(SnairEngine::mutated(currentParams(), nextVariationSeed()));
}

void SnairCreatorProcessor::resetParameters()
{
    pushUndo();
    SnairParameters defaults;
    defaults.mode = currentParams().mode; // reset keeps Snare/Clap selection
    applyParameters(defaults);
}

void SnairCreatorProcessor::loadParametersAsAction(const SnairParameters& p)
{
    pushUndo();
    applyParameters(p);
}

bool SnairCreatorProcessor::undo()
{
    if (undoHistory.empty()) return false;
    auto entry = undoHistory.back();
    undoHistory.pop_back();
    applyParameters(entry.params);
    // Restore the cached sound instantly when it matches the restored settings and current source.
    if (entry.hit && entry.hit != hit && (!analysis || entry.hit->sourceFingerprint == analysis->fingerprint)
        && std::abs(entry.hit->sampleRate - hostRate.load()) < 0.5)
    {
        latestRequestId = 0;
        requestedHash = entry.hit->paramHash;
        publish(entry.hit);
        setStatus(Status::ready, "Undo restored the previous sound (seed " + juce::String(entry.hit->seed) + ").");
    }
    return true;
}

//==============================================================================
juce::String SnairCreatorProcessor::defaultExportName(const juce::String& suffix) const
{
    const auto p = currentParams();
    const auto seed = hit ? hit->seed : p.seed;
    return "SnairCreator_" + juce::String(p.mode == SnairMode::clap ? "Clap" : "Snare") + "_"
         + sanitizeFilePart(sourceName) + "_" + juce::String(seed).paddedLeft('0', 4) + suffix + ".wav";
}

bool SnairCreatorProcessor::exportHit(const juce::File& file, juce::String& message) const
{
    if (!hit) { message = "Nothing to export yet: load a source first."; return false; }
    auto opt = exportOptions;
    opt.outputTrimDb = currentParams().outputTrimDb;
    if (!WavExporter::write(file, *hit, opt, message)) return false;
    const double rate = opt.sampleRate > 0.0 ? opt.sampleRate : hit->sampleRate;
    message = "Exported " + file.getFullPathName() + "  (" + juce::String(hit->samples.getNumSamples() / hit->sampleRate, 2) + " s, "
            + juce::String(rate / 1000.0, 1) + " kHz, " + (opt.bitDepth == 32 ? juce::String("32-bit float") : juce::String(opt.bitDepth) + "-bit")
            + (opt.channels == 1 ? ", mono)" : ", stereo)");
    return true;
}

juce::File SnairCreatorProcessor::writeDragFile(juce::String& error) const
{
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("SnairCreator");
    dir.createDirectory();
    auto file = dir.getChildFile(defaultExportName());
    juce::String msg;
    if (!exportHit(file, msg)) { error = msg; return {}; }
    return file;
}

void SnairCreatorProcessor::exportKit(const juce::File& folder, int count, std::function<void(juce::String)> onDone)
{
    if (!source || !analysis) { onDone("Load a source before exporting a kit."); return; }
    const auto base = currentParams();
    const auto rate = hit ? hit->sampleRate : hostRate.load();
    auto opt = exportOptions;
    opt.outputTrimDb = base.outputTrimDb;
    const auto modeName = juce::String(base.mode == SnairMode::clap ? "Clap" : "Snare");
    const auto prefix = "SnairCreator_" + modeName + "_" + sanitizeFilePart(sourceName);
    auto s = source; auto a = analysis;
    const uint32_t kitSeed = nextVariationSeed();
    jobs.addJob([=]
    {
        juce::String error;
        int written = 0;
        folder.createDirectory();
        for (int i = 0; i < count; ++i)
        {
            // Variation 1 is the current sound; the rest are mutations of it (a coherent kit).
            auto p = i == 0 ? base : SnairEngine::mutated(base, kitSeed + static_cast<uint32_t>(i) * 7919u);
            auto h = SnairEngine::render(*s, *a, p, 0, error, rate);
            if (!h) break;
            auto file = folder.getChildFile(prefix + "_" + juce::String(i + 1).paddedLeft('0', 2) + "_" + juce::String(p.seed).paddedLeft('0', 4) + ".wav")
                              .getNonexistentSibling();
            if (!WavExporter::write(file, *h, opt, error)) break;
            ++written;
        }
        const auto msg = written == count ? "Kit exported: " + juce::String(written) + " WAV variations in " + folder.getFullPathName()
                                          : "Kit export stopped after " + juce::String(written) + " files: " + error;
        juce::MessageManager::callAsync([onDone, msg] { onDone(msg); });
    });
}

//==============================================================================
void SnairCreatorProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    RenderedHitPtr hitSnapshot;
    juce::File fileSnapshot;
    juce::String nameSnapshot;
    uint64_t fingerprintSnapshot;
    {
        // Hosts may request state off the message thread; snapshot shared members under the lock.
        std::lock_guard<std::mutex> l(stateLock);
        hitSnapshot = hit; fileSnapshot = sourceFile; nameSnapshot = sourceName; fingerprintSnapshot = savedSourceFingerprint;
    }
    const auto& h0 = hitSnapshot;
    juce::ValueTree root("SnairCreatorState");
    root.setProperty("version", 1, nullptr);
    root.appendChild(apvts.copyState(), nullptr);

    juce::ValueTree session("Session");
    session.setProperty("sourcePath", fileSnapshot.getFullPathName(), nullptr);
    session.setProperty("sourceName", nameSnapshot, nullptr);
    session.setProperty("sourceFingerprint", juce::String::toHexString(static_cast<juce::int64>(fingerprintSnapshot)), nullptr);
    session.setProperty("exportBits", exportOptions.bitDepth, nullptr);
    session.setProperty("exportRate", exportOptions.sampleRate, nullptr);
    session.setProperty("exportChannels", exportOptions.channels, nullptr);
    session.setProperty("exportNormalize", exportOptions.normalize, nullptr);
    session.setProperty("exportTrim", exportOptions.includeOutputTrim, nullptr);
    session.setProperty("tooltips", tooltipsEnabled, nullptr);
    root.appendChild(session, nullptr);

    if (h0 && h0->samples.getNumSamples() <= maxStateHitSamples)
    {
        // The current rendered hit is embedded so the session recalls the exact sound even if the source moved.
        juce::ValueTree h("Hit");
        h.setProperty("sampleRate", h0->sampleRate, nullptr);
        h.setProperty("seed", static_cast<int>(h0->seed), nullptr);
        h.setProperty("paramHash", juce::String::toHexString(static_cast<juce::int64>(h0->paramHash)), nullptr);
        h.setProperty("fingerprint", juce::String::toHexString(static_cast<juce::int64>(h0->sourceFingerprint)), nullptr);
        h.setProperty("attack", static_cast<juce::int64>(h0->attackSourceSample), nullptr);
        h.setProperty("fallback", h0->usedFallback, nullptr);
        juce::MemoryBlock data;
        for (int ch = 0; ch < 2; ++ch)
            data.append(h0->samples.getReadPointer(ch), sizeof(float) * static_cast<size_t>(h0->samples.getNumSamples()));
        h.setProperty("frames", h0->samples.getNumSamples(), nullptr);
        h.setProperty("data", data, nullptr);
        root.appendChild(h, nullptr);
    }
    juce::MemoryOutputStream out(dest, false);
    root.writeToStream(out);
}

void SnairCreatorProcessor::setStateInformation(const void* data, int size)
{
    if (data == nullptr || size <= 0 || size > 64 * 1024 * 1024) return;
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(size));
    if (!tree.isValid() || !tree.hasType("SnairCreatorState") || static_cast<int>(tree.getProperty("version", 0)) > 1)
        return; // malformed or future state: keep current state untouched
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        restoreFromTree(tree);
    else
    {
        juce::WeakReference<SnairCreatorProcessor> weak(this);
        juce::MessageManager::callAsync([weak, tree] { if (auto* self = weak.get()) self->restoreFromTree(tree); });
    }
}

namespace
{
uint64_t hex64(const juce::var& v) { return static_cast<uint64_t>(v.toString().getHexValue64()); }
}

void SnairCreatorProcessor::restoreFromTree(const juce::ValueTree& tree)
{
    const auto params = tree.getChildWithName(apvts.state.getType());
    if (params.isValid()) apvts.replaceState(params);

    const auto session = tree.getChildWithName("Session");
    exportOptions.bitDepth = session.getProperty("exportBits", 24);
    exportOptions.sampleRate = session.getProperty("exportRate", 0.0);
    exportOptions.channels = juce::jlimit(1, 2, static_cast<int>(session.getProperty("exportChannels", 2)));
    exportOptions.normalize = session.getProperty("exportNormalize", false);
    exportOptions.includeOutputTrim = session.getProperty("exportTrim", true);
    tooltipsEnabled = session.getProperty("tooltips", true);

    const auto h = tree.getChildWithName("Hit");
    if (h.isValid())
    {
        const int frames = h.getProperty("frames", 0);
        const auto* block = h.getProperty("data").getBinaryData();
        const double rate = h.getProperty("sampleRate", 0.0);
        if (block != nullptr && frames > 0 && frames <= maxStateHitSamples && rate >= 8000.0 && rate <= 384000.0
            && block->getSize() == sizeof(float) * 2 * static_cast<size_t>(frames))
        {
            auto restored = std::make_shared<RenderedHit>();
            restored->sampleRate = rate;
            restored->seed = static_cast<uint32_t>(static_cast<int>(h.getProperty("seed", 0)));
            restored->paramHash = hex64(h.getProperty("paramHash"));
            restored->sourceFingerprint = hex64(h.getProperty("fingerprint"));
            restored->attackSourceSample = static_cast<juce::int64>(h.getProperty("attack", 0));
            restored->usedFallback = h.getProperty("fallback", false);
            restored->samples.setSize(2, frames);
            const auto* f = static_cast<const float*>(block->getData());
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* d = restored->samples.getWritePointer(ch);
                for (int i = 0; i < frames; ++i) d[i] = std::isfinite(f[ch * frames + i]) ? juce::jlimit(-4.0f, 4.0f, f[ch * frames + i]) : 0.0f;
            }
            restored->peak = restored->samples.getMagnitude(0, frames);
            constexpr int bins = 400;
            restored->peaks.assign(bins, 0.0f);
            for (int i = 0; i < frames; ++i)
            {
                auto& b = restored->peaks[std::min<size_t>(bins - 1, static_cast<size_t>(static_cast<int64_t>(i) * bins / frames))];
                b = std::max({ b, std::abs(restored->samples.getSample(0, i)), std::abs(restored->samples.getSample(1, i)) });
            }
            latestRequestId = 0;
            requestedHash = restored->paramHash;
            requestedRate = restored->sampleRate;
            publish(restored);
        }
    }

    const auto pathText = session.getProperty("sourcePath").toString();
    const juce::File path = juce::File::isAbsolutePath(pathText) ? juce::File(pathText) : juce::File();
    source.reset();
    analysis.reset();
    {
        std::lock_guard<std::mutex> l(stateLock);
        sourceName = session.getProperty("sourceName").toString();
        savedSourceFingerprint = hex64(session.getProperty("sourceFingerprint"));
        sourceFile = path;
    }
    if (path.getFullPathName().isNotEmpty() && path.existsAsFile())
    {
        sourceMissing = true; // cleared when the reload completes with a matching fingerprint
        const auto token = ++sourceLoadToken;
        setStatus(hit ? Status::ready : Status::loading, "Session restored. Re-reading source " + path.getFileName() + "...");
        juce::WeakReference<SnairCreatorProcessor> weak(this);
        jobs.addJob([weak, path, token]
        {
            juce::String error;
            auto decoded = SourceLoader::decode(path, error);
            auto result = std::make_shared<SourceAnalysis>();
            if (decoded && !SourceAnalyzer::analyze(*decoded, *result, error)) decoded.reset();
            juce::MessageManager::callAsync([weak, decoded, result, token]
            {
                auto* self = weak.get();
                if (self == nullptr || token != self->sourceLoadToken) return;
                if (decoded && (self->savedSourceFingerprint == 0 || result->fingerprint == self->savedSourceFingerprint))
                    self->installSource(decoded, result, true);
                else
                    self->setStatus(Status::error, "The original source changed or could not be read. The saved hit is kept and playable; load the source again to regenerate.");
            });
        });
    }
    else if (path.getFullPathName().isNotEmpty())
    {
        sourceMissing = true;
        setStatus(hit ? Status::ready : Status::error, "Original source missing (" + path.getFileName()
                  + "). The saved hit is playable and exportable; load/relink a source to regenerate.");
    }
    else
        setStatus(hit ? Status::ready : Status::empty, hit ? "Session restored." : "Load or drop a WAV/AIFF source to begin.");
    paramsDirty.store(false);
}

juce::AudioProcessorEditor* SnairCreatorProcessor::createEditor() { return new SnairCreatorEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SnairCreatorProcessor(); }
