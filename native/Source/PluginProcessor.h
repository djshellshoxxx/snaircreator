#pragma once
#include <JuceHeader.h>
#include "SnairEngine.h"
#include <array>
#include <vector>

class SnairCreatorAudioProcessor : public juce::AudioProcessor,
                                   private juce::AudioProcessorValueTreeState::Listener,
                                   private juce::AsyncUpdater {
public:
    SnairCreatorAudioProcessor();
    ~SnairCreatorAudioProcessor() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool loadSourceFile(const juce::File& file);
    bool exportRendered(const juce::File& file, int bitDepth = 24) const;
    void rebuildRendered();
    void triggerPreview() { previewRequested.store(true); }
    juce::String getSourceDescription() const;
    std::vector<float> getSourcePreview(int points) const;
    snair::Analysis getAnalysis() const { return analysis; }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct Voice { bool active = false; size_t position = 0; float velocity = 1.0f; };
    std::array<Voice, 16> voices {};
    std::vector<float> source;
    snair::Analysis analysis;
    juce::File sourceFile;
    juce::String missingSourcePath;
    double renderSampleRate = 48000.0;
    mutable juce::SpinLock renderedLock;
    std::vector<float> rendered;
    std::atomic<bool> previewRequested { false };

    snair::Params getParams() const;
    void startVoice(float velocity);
    void parameterChanged(const juce::String&, float) override;
    void handleAsyncUpdate() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnairCreatorAudioProcessor)
};
