#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include "AdvancedPanel.h"
#include "Parameters.h"
#include "PresetManager.h"
#include "RenderedHit.h"
#include "SessionStore.h"
#include "SourceAnalysis.h"
#include "SourceAudio.h"
#include "WaveformView.h"

class MainComponent final : public juce::AudioAppComponent,
                            public juce::FileDragAndDropTarget,
                            private juce::AsyncUpdater
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
    void prepareToPlay(int, double) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
    void releaseResources() override;

private:
    static constexpr int macroCount=6;

    void handleAsyncUpdate() override;
    void configureButton(juce::TextButton&);
    void configureSlider(juce::Slider&,double,double,double);
    void chooseSource();
    void loadSource(const juce::File&);
    void renderCurrent(const juce::String& reason);
    void syncControlsFromParameters();
    void syncParametersFromControls();
    void setMode(bool clap);
    void startPreview();
    void randomize();
    void mutate();
    void undo();
    void resetParameters();
    void exportWav();
    void savePreset();
    void savePresetAs();
    void loadPreset();
    void applyFactoryPreset(int index);
    void refreshActionState();
    void toggleAdvanced();
    void restoreSession();
    void saveSessionAsync(const SnairParameters&, const RenderedHitPtr&, const juce::File&);

    juce::Label titleLabel, buildLabel, sectionSourceLabel, sourceNameLabel, sourceDetailsLabel, sourceHintLabel;
    juce::Label modeLabel, characterLabel, statusLabel;
    WaveformView waveform;
    juce::TextButton loadSourceButton{"LOAD SOURCE"}, snareButton{"SNARE"}, clapButton{"CLAP"};
    juce::TextButton previewButton{"PREVIEW"}, randomizeButton{"RANDOMIZE"}, mutateButton{"MUTATE"}, undoButton{"UNDO"}, resetButton{"RESET"};
    juce::TextButton exportButton{"EXPORT WAV"}, savePresetButton{"SAVE"}, savePresetAsButton{"SAVE AS"}, loadPresetButton{"LOAD"};
    juce::TextButton advancedButton{"ADVANCED"};
    juce::ComboBox presetSelector;
    AdvancedPanel advancedPanel;
    juce::Slider sourceCharacterSlider;
    std::array<juce::Slider,macroCount> macroSliders;
    std::array<juce::Label,macroCount> macroLabels;
    std::array<juce::String,macroCount> macroNames{"PUNCH","SNAP","BODY","TEXTURE","DIRT","SIZE"};

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File currentPresetFile;
    SourceAudioPtr source;
    SourceAnalysis analysis;
    SnairParameters parameters;
    SnairParameters undoParameters;
    bool hasUndo=false;
    bool advancedVisible=false;

    juce::ThreadPool sourceWorker{1};
    juce::ThreadPool renderWorker{1};
    juce::ThreadPool sessionWorker{1};
    std::atomic<uint64_t> sourceRequest{0};
    std::atomic<uint64_t> renderRequest{0};
    std::atomic<bool> recoverySuperseded{false};
    std::atomic<RenderedHitPtr> renderedHit{};
    std::atomic<int> previewPosition{0};
    std::atomic<bool> previewActive{false};
    std::atomic<double> playbackSampleRate{48000.0};
    std::atomic<float> playbackTrimGain{1.0f};
    uint64_t generation=0;
    uint32_t variationCounter=100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};