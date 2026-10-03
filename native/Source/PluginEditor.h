#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <vector>

class SnairCreatorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::FileDragAndDropTarget,
                                        private juce::Timer {
public:
    explicit SnairCreatorAudioProcessorEditor(SnairCreatorAudioProcessor&);
    ~SnairCreatorAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void filesDropped(const juce::StringArray&, int, int) override;

private:
    struct Control {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    SnairCreatorAudioProcessor& processor;
    juce::Label title, subtitle, sourceInfo;
    juce::ComboBox mode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    juce::ToggleButton normalize { "Normalize" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> normalizeAttachment;
    juce::TextButton load { "Load source" }, preview { "Preview" }, randomize { "Randomize" }, mutate { "Mutate" }, undo { "Undo mutation" }, reset { "Reset" }, exportWav { "Export WAV" }, savePreset { "Save preset" }, loadPreset { "Load preset" };
    std::array<juce::Slider, 4> macros;
    std::array<juce::Label, 4> macroLabels;
    std::vector<std::unique_ptr<Control>> controls;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::MemoryBlock undoState;
    juce::Rectangle<int> waveformBounds;

    void addControl(const juce::String& name, const juce::String& id);
    void chooseSource();
    void chooseExport();
    void choosePresetSave();
    void choosePresetLoad();
    void randomizeParameters(bool subtle);
    void resetParameters();
    void captureUndo();
    void restoreUndo();
    void applyMacro(int index, float value);
    void setParameterActual(const char* id, float actual);
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnairCreatorAudioProcessorEditor)
};
