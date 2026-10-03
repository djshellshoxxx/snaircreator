#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
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
    juce::TextButton load { "Load source" }, preview { "Preview" }, randomize { "Randomize" }, mutate { "Mutate" }, reset { "Reset" }, exportWav { "Export WAV" };
    std::vector<std::unique_ptr<Control>> controls;
    std::unique_ptr<juce::FileChooser> chooser;
    void addControl(const juce::String& name, const juce::String& id);
    void chooseSource();
    void chooseExport();
    void randomizeParameters(bool subtle);
    void resetParameters();
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnairCreatorAudioProcessorEditor)
};
