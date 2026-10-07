#pragma once
#include <JuceHeader.h>
#include "HitPlayer.h"
#include "Parameters.h"
#include "RenderedHit.h"
#include "SourceAnalysis.h"
#include "SourceAudio.h"
#include "WavExporter.h"
#include <atomic>
#include <deque>
#include <mutex>

class SnairCreatorProcessor final : public juce::AudioProcessor,
                                    public juce::ChangeBroadcaster,
                                    private juce::AudioProcessorValueTreeState::Listener,
                                    private juce::AsyncUpdater,
                                    private juce::Timer
{
public:
    enum class Status { empty, loading, rendering, ready, error };

    SnairCreatorProcessor();
    ~SnairCreatorProcessor() override;

    // AudioProcessor
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SnairCreator"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Message-thread controller API used by the editor.
    juce::AudioProcessorValueTreeState& state() noexcept { return apvts; }
    SnairParameters currentParams() const;
    void applyParameters(const SnairParameters&);
    void loadSource(const juce::File&);
    void triggerPreview() noexcept { previewRequests.fetch_add(1); }
    void randomize();
    void mutate();
    bool undo();
    bool canUndo() const noexcept { return !undoHistory.empty(); }
    void resetParameters();
    void loadParametersAsAction(const SnairParameters&); // preset load (undoable)

    bool exportHit(const juce::File&, juce::String& message) const;
    juce::File writeDragFile(juce::String& error) const;  // temp WAV for drag-out
    void exportKit(const juce::File& folder, int count, std::function<void(juce::String)> onDone);
    juce::String defaultExportName(const juce::String& suffix = {}) const;

    RenderedHitPtr currentHit() const noexcept { return hit; }
    SourceAudioPtr currentSource() const noexcept { return source; }
    const SourceAnalysis* currentAnalysis() const noexcept { return analysis.get(); }
    Status getStatus() const noexcept { return status; }
    juce::String getStatusText() const { return statusText; }
    juce::String getSourceName() const { return sourceName; }
    bool isSourceMissing() const noexcept { return sourceMissing; }
    bool isHitStale() const; // visible params differ from the active hit
    uint32_t midiActivityCount() const noexcept { return midiActivity.load(); }
    int activeVoices() const noexcept { return voicesPlaying.load(); }

    WavExportOptions exportOptions;
    bool tooltipsEnabled = true;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    struct Request { SourceAudioPtr source; std::shared_ptr<const SourceAnalysis> analysis; SnairParameters params; double rate; uint64_t id; };
    struct Result { std::shared_ptr<RenderedHit> hit; juce::String error; uint64_t id = 0; };
    class Worker;

    void parameterChanged(const juce::String&, float) override { paramsDirty.store(true); }
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void requestRenderIfNeeded(bool force = false);
    void publish(RenderedHitPtr);
    void collectGarbage();
    void pushUndo();
    void setStatus(Status, const juce::String&);
    void installSource(SourceAudioPtr, std::shared_ptr<const SourceAnalysis>, bool keepRestoredHit);
    void restoreFromTree(const juce::ValueTree&);
    uint32_t nextVariationSeed();

    juce::AudioProcessorValueTreeState apvts;
    HitPlayer player;
    std::unique_ptr<Worker> worker;
    juce::ThreadPool jobs { 1 };

    // Message-thread owned state.
    SourceAudioPtr source;
    std::shared_ptr<const SourceAnalysis> analysis;
    juce::File sourceFile;
    juce::String sourceName;
    uint64_t savedSourceFingerprint = 0;
    bool sourceMissing = false;
    RenderedHitPtr hit;
    struct Retired { RenderedHitPtr hit; uint64_t block; };
    std::vector<Retired> retired;
    struct UndoEntry { SnairParameters params; RenderedHitPtr hit; };
    std::deque<UndoEntry> undoHistory;
    Status status = Status::empty;
    juce::String statusText { "Load or drop a WAV/AIFF source to begin." };
    uint64_t requestedHash = 0;
    double requestedRate = 0.0;
    uint64_t nextRequestId = 1;
    uint64_t latestRequestId = 0;
    uint32_t variationCounter = 1;
    uint64_t sourceLoadToken = 0;

    // Cross-thread.
    std::mutex resultLock;
    mutable std::mutex stateLock;
    Result completed;
    std::atomic<bool> paramsDirty { true };
    std::atomic<double> hostRate { 48000.0 };
    std::atomic<bool> audioPrepared { false };
    std::atomic<uint64_t> blockCounter { 0 };
    std::array<std::atomic<const RenderedHit*>, HitPlayer::maxVoices + 1> inUse {};
    std::atomic<int> previewRequests { 0 };
    std::atomic<uint32_t> midiActivity { 0 };
    std::atomic<int> voicesPlaying { 0 };
    std::atomic<float>* trimParam = nullptr;
    std::atomic<float>* keyTrackParam = nullptr;
    float lastTrimGain = 1.0f;

    JUCE_DECLARE_WEAK_REFERENCEABLE(SnairCreatorProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnairCreatorProcessor)
};
