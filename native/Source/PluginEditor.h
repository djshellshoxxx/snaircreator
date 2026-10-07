#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WaveformView.h"
#include <array>

class SnairLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SnairLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
};

// Slider with fine mouse-wheel steps (1% per notch, 0.1% with Shift) and a context menu
// offering value entry and reset-to-default.
class FineSlider final : public juce::Slider
{
public:
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDown(const juce::MouseEvent&) override;
};

class SnairCreatorEditor final : public juce::AudioProcessorEditor,
                                 public juce::FileDragAndDropTarget,
                                 public juce::DragAndDropContainer,
                                 private juce::ChangeListener,
                                 private juce::Timer
{
public:
    explicit SnairCreatorEditor(SnairCreatorProcessor&);
    ~SnairCreatorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    static constexpr int macroCount = 6, advCount = 13;

    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
    void timerCallback() override;
    void refresh();
    void setupSlider(FineSlider&, const char* paramId, const juce::String& tip, bool rotary);
    void setupButton(juce::Button&, const juce::String& tip);
    void chooseSource();
    void exportWav();
    void exportKit();
    void dragHitOut();
    void savePreset(bool forceChooser);
    void loadPresetFile();
    void rebuildPresetList();
    void presetChosen();
    void showOptions();
    void setHelpVisible(bool);
    void applyTooltips();

    SnairCreatorProcessor& proc;
    SnairLookAndFeel lnf;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> sourcePanel, modePanel, macroPanel;

    juce::Label title, version, sourceHeading, sourceName, sourceDetails, analysisDetails, statusLabel, midiLabel, charLabel, roomLabel;
    WaveformView waveform;
    juce::TextButton loadSourceButton { "LOAD SOURCE" }, viewButton { "SHOW HIT" };
    juce::TextButton snareButton { "SNARE" }, clapButton { "CLAP" };
    juce::TextButton previewButton { "PREVIEW" }, randomizeButton { "RANDOMIZE" }, mutateButton { "MUTATE" },
                     undoButton { "UNDO" }, resetButton { "RESET" }, advancedButton { "ADVANCED" };
    juce::TextButton exportButton { "EXPORT WAV" }, kitButton { "EXPORT KIT" };
    juce::TextButton savePresetButton { "SAVE" }, saveAsButton { "SAVE AS" }, loadPresetButton { "LOAD" },
                     optionsButton { "OPTIONS" }, helpButton { "HELP" }, closeHelpButton { "CLOSE HELP" };
    juce::ToggleButton keyTrackToggle { "KEY TRACK" }, normalizeToggle { "Normalize render" },
                       exportNormalize { "Normalize export" }, exportTrim { "Include output trim" };
    juce::ComboBox presetBox, bitDepthBox, rateBox, channelsBox;
    FineSlider sourceCharacter, roomSlider;
    std::array<FineSlider, macroCount> macros;
    std::array<juce::Label, macroCount> macroLabels;
    std::array<FineSlider, advCount> adv;
    std::array<juce::Label, advCount> advLabels;
    juce::Label exportHeading;
    juce::TextEditor helpText;
    juce::Label helpBackdrop;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

    juce::File currentPresetFile;
    juce::Array<juce::File> userPresets;
    bool advancedVisible = false;
    uint32_t lastMidi = 0;
    int midiFlash = 0;
    const RenderedHit* shownHit = nullptr;
    const SourceAudio* shownSource = nullptr;
    juce::String transientMessage;
    int transientTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnairCreatorEditor)
};
