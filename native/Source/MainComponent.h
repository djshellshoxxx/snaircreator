#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

class MainComponent final : public juce::Component,
                            public juce::FileDragAndDropTarget
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    static constexpr int macroCount = 6;

    void configureButton(juce::TextButton& button);
    void chooseSource();
    void loadSourceMetadata(const juce::File& file);
    void setMode(bool clapSelected);
    bool hasSupportedExtension(const juce::File& file) const;
    void showHelp();
    void showOptions();
    void applyTooltipSetting();

    juce::Label titleLabel;
    juce::Label buildLabel;
    juce::Label sectionSourceLabel;
    juce::Label sourceNameLabel;
    juce::Label sourceDetailsLabel;
    juce::Label sourceHintLabel;
    juce::Label modeLabel;
    juce::Label characterLabel;
    juce::Label statusLabel;

    juce::TextButton loadSourceButton { "LOAD SOURCE" };
    juce::TextButton snareButton { "SNARE" };
    juce::TextButton clapButton { "CLAP" };
    juce::TextButton previewButton { "PREVIEW" };
    juce::TextButton mutateButton { "MUTATE" };
    juce::TextButton undoButton { "UNDO" };
    juce::TextButton resetButton { "RESET" };
    juce::TextButton randomizeButton { "RANDOMIZE" };
    juce::TextButton exportButton { "EXPORT WAV" };
    juce::TextButton optionsButton { "OPTIONS" };
    juce::TextButton helpButton { "HELP" };
    juce::TextButton closeHelpButton { "CLOSE HELP" };

    juce::Slider sourceCharacterSlider;
    std::array<juce::Slider, macroCount> macroSliders;
    std::array<juce::Label, macroCount> macroLabels;
    std::array<juce::String, macroCount> macroNames {
        "PUNCH", "SNAP", "BODY", "TEXTURE", "DIRT", "SIZE"
    };

    juce::TextEditor helpText;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    bool tooltipsEnabled = true;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File currentSource;
    bool clapMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
