#include "MainComponent.h"

namespace
{
const juce::Colour backgroundColour { 15, 18, 23 };
const juce::Colour panelColour { 24, 29, 37 };
const juce::Colour borderColour { 48, 58, 70 };
const juce::Colour accentColour { 47, 202, 224 };
const juce::Colour mutedColour { 145, 157, 171 };
}

MainComponent::MainComponent()
{
    formatManager.registerBasicFormats();

    titleLabel.setText("SnairCreator", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(27.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    buildLabel.setText("STANDALONE  /  EARLY BUILD", juce::dontSendNotification);
    buildLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    buildLabel.setColour(juce::Label::textColourId, accentColour);
    addAndMakeVisible(buildLabel);

    configureButton(optionsButton);
    configureButton(helpButton);
    optionsButton.setTooltip("Open interface options, including the global tooltip switch.");
    helpButton.setTooltip("Open the complete SnairCreator help section.");
    optionsButton.onClick = [this] { showOptions(); };
    helpButton.onClick = [this] { showHelp(); };
    addAndMakeVisible(optionsButton);
    addAndMakeVisible(helpButton);

    helpText.setMultiLine(true);
    helpText.setReadOnly(true);
    helpText.setScrollbarsShown(true);
    helpText.setCaretVisible(false);
    helpText.setColour(juce::TextEditor::backgroundColourId, panelColour);
    helpText.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    helpText.setColour(juce::TextEditor::outlineColourId, accentColour);
    helpText.setText(
        "SNAIRCREATOR HELP\n\n"
        "GETTING STARTED\n"
        "Load or drag one WAV/AIFF source into the window. Choose Snare or Clap as the target character. "
        "The source header is validated immediately.\n\n"
        "SOURCE CHARACTER\n"
        "This control determines how strongly the generated result keeps recognizable traits from the source. "
        "It becomes active when the render engine is available.\n\n"
        "MACROS\n"
        "Punch controls transient impact, Snap controls the upper transient, Body controls weight, Texture changes "
        "surface/noise detail, Dirt adds roughness, and Size changes perceived scale/decay. Disabled controls indicate "
        "a feature that is not connected in this early build.\n\n"
        "ACTIONS\n"
        "Preview auditions the generated sound. Randomize creates a fresh variation. Mutate makes a smaller change. "
        "Undo restores the previous mutation. Reset restores defaults. Export WAV writes the rendered result.\n\n"
        "FILES\n"
        "Supported input formats in this build are WAV, AIF and AIFF. Drop exactly one file at a time. An invalid or "
        "unreadable source leaves the previous valid source unchanged.\n\n"
        "TOOLTIPS\n"
        "Hover a control for contextual help. OPTIONS > Show tooltips enables or disables all hover tips.\n\n"
        "TROUBLESHOOTING\n"
        "If a control is disabled, the corresponding render/playback feature is not yet connected. If a file is rejected, "
        "confirm it is a valid WAV/AIFF file with readable audio data.\n");
    helpText.setVisible(false);
    addChildComponent(helpText);

    configureButton(closeHelpButton);
    closeHelpButton.setTooltip("Close this help section.");
    closeHelpButton.onClick = [this] { helpText.setVisible(false); closeHelpButton.setVisible(false); };
    closeHelpButton.setVisible(false);
    addChildComponent(closeHelpButton);

    applyTooltipSetting();

    sectionSourceLabel.setText("SOURCE AUDIO", juce::dontSendNotification);
    sectionSourceLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    sectionSourceLabel.setColour(juce::Label::textColourId, accentColour);
    addAndMakeVisible(sectionSourceLabel);

    sourceNameLabel.setText("No source loaded", juce::dontSendNotification);
    sourceNameLabel.setFont(juce::Font(17.0f, juce::Font::bold));
    sourceNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(sourceNameLabel);

    sourceDetailsLabel.setText("WAV or AIFF  •  One file at a time", juce::dontSendNotification);
    sourceDetailsLabel.setFont(juce::Font(12.0f));
    sourceDetailsLabel.setColour(juce::Label::textColourId, mutedColour);
    addAndMakeVisible(sourceDetailsLabel);

    sourceHintLabel.setText("Drop an audio file here to inspect it", juce::dontSendNotification);
    sourceHintLabel.setFont(juce::Font(12.0f));
    sourceHintLabel.setColour(juce::Label::textColourId, mutedColour);
    addAndMakeVisible(sourceHintLabel);

    configureButton(loadSourceButton);
    loadSourceButton.setColour(juce::TextButton::buttonColourId, accentColour.withAlpha(0.16f));
    loadSourceButton.setColour(juce::TextButton::textColourOffId, accentColour);
    loadSourceButton.setTooltip("Choose a WAV or AIFF source file. You can also drag one file into the window.");
    loadSourceButton.onClick = [this] { chooseSource(); };
    addAndMakeVisible(loadSourceButton);

    modeLabel.setText("OUTPUT CHARACTER", juce::dontSendNotification);
    modeLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    modeLabel.setColour(juce::Label::textColourId, accentColour);
    addAndMakeVisible(modeLabel);

    configureButton(snareButton);
    configureButton(clapButton);
    snareButton.setRadioGroupId(1);
    clapButton.setRadioGroupId(1);
    snareButton.setClickingTogglesState(true);
    clapButton.setClickingTogglesState(true);
    snareButton.setToggleState(true, juce::dontSendNotification);
    snareButton.setTooltip("Generate toward a snare-drum character.");
    clapButton.setTooltip("Generate toward a clap character.");
    snareButton.onClick = [this] { setMode(false); };
    clapButton.onClick = [this] { setMode(true); };
    addAndMakeVisible(snareButton);
    addAndMakeVisible(clapButton);

    characterLabel.setText("SOURCE CHARACTER", juce::dontSendNotification);
    characterLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    characterLabel.setColour(juce::Label::textColourId, mutedColour);
    addAndMakeVisible(characterLabel);

    sourceCharacterSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sourceCharacterSlider.setRange(0.0, 1.0, 0.01);
    sourceCharacterSlider.setValue(0.5);
    sourceCharacterSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 22);
    sourceCharacterSlider.setEnabled(false);
    sourceCharacterSlider.setTooltip("Available after the render engine is connected.");
    addAndMakeVisible(sourceCharacterSlider);

    for (int i = 0; i < macroCount; ++i)
    {
        auto& label = macroLabels[static_cast<size_t>(i)];
        label.setText(macroNames[static_cast<size_t>(i)], juce::dontSendNotification);
        label.setFont(juce::Font(11.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, mutedColour);
        addAndMakeVisible(label);

        auto& slider = macroSliders[static_cast<size_t>(i)];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 20);
        slider.setRange(0.0, 1.0, 0.01);
        slider.setValue(0.5);
        slider.setEnabled(false);
        slider.setTooltip("Available after the render engine is connected.");
        addAndMakeVisible(slider);
    }

    configureButton(previewButton);
    configureButton(mutateButton);
    configureButton(undoButton);
    configureButton(resetButton);
    configureButton(randomizeButton);
    configureButton(exportButton);

    previewButton.setEnabled(false);
    randomizeButton.setEnabled(false);
    mutateButton.setEnabled(false);
    undoButton.setEnabled(false);
    resetButton.setEnabled(false);
    exportButton.setEnabled(false);

    previewButton.setTooltip("Preview is enabled when rendering and playback are implemented.");
    randomizeButton.setTooltip("Randomize is available after the render engine is implemented.");
    mutateButton.setTooltip("Mutate is available after the render engine is implemented.");
    undoButton.setTooltip("Undo is available after mutation history is implemented.");
    resetButton.setTooltip("Reset is available after the render engine is implemented.");
    exportButton.setTooltip("WAV export is available after a sound has been rendered.");

    addAndMakeVisible(previewButton);
    addAndMakeVisible(randomizeButton);
    addAndMakeVisible(mutateButton);
    addAndMakeVisible(undoButton);
    addAndMakeVisible(resetButton);
    addAndMakeVisible(exportButton);

    statusLabel.setText("App shell ready. Sound generation and playback are not connected yet.",
                        juce::dontSendNotification);
    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setColour(juce::Label::textColourId, mutedColour);
    addAndMakeVisible(statusLabel);

    setSize(1000, 700);
}

void MainComponent::applyTooltipSetting()
{
    if (tooltipsEnabled)
    {
        if (tooltipWindow == nullptr)
            tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 600);
    }
    else
    {
        tooltipWindow.reset();
    }
}

void MainComponent::showOptions()
{
    juce::PopupMenu menu;
    menu.addSectionHeader("Interface");
    menu.addItem(1, "Show tooltips", true, tooltipsEnabled);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&optionsButton),
                       [this](int result)
                       {
                           if (result == 1)
                           {
                               tooltipsEnabled = !tooltipsEnabled;
                               applyTooltipSetting();
                           }
                       });
}

void MainComponent::showHelp()
{
    helpText.setVisible(true);
    closeHelpButton.setVisible(true);
    resized();
    helpText.toFront(false);
    closeHelpButton.toFront(false);
}

void MainComponent::configureButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, panelColour);
    button.setColour(juce::TextButton::buttonOnColourId, accentColour.withAlpha(0.20f));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    button.setColour(juce::TextButton::textColourOnId, accentColour);
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(backgroundColour);

    const bool compact = getHeight() < 600;
    const int headerHeight = compact ? 44 : 52;
    const int gap = compact ? 8 : 12;
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(headerHeight + gap);

    auto sourceArea = area.removeFromTop(compact ? 122 : 146);
    graphics.setColour(panelColour);
    graphics.fillRoundedRectangle(sourceArea.toFloat(), 10.0f);
    graphics.setColour(borderColour);
    graphics.drawRoundedRectangle(sourceArea.toFloat(), 10.0f, 1.0f);

    area.removeFromTop(gap);
    auto modeArea = area.removeFromTop(compact ? 74 : 90);
    graphics.setColour(panelColour);
    graphics.fillRoundedRectangle(modeArea.toFloat(), 10.0f);
    graphics.setColour(borderColour);
    graphics.drawRoundedRectangle(modeArea.toFloat(), 10.0f, 1.0f);

    area.removeFromTop(gap);
    auto controlsArea = area.removeFromTop(compact ? 148 : 226);
    graphics.setColour(panelColour);
    graphics.fillRoundedRectangle(controlsArea.toFloat(), 10.0f);
    graphics.setColour(borderColour);
    graphics.drawRoundedRectangle(controlsArea.toFloat(), 10.0f, 1.0f);
}

void MainComponent::resized()
{
    const bool compact = getHeight() < 600;
    const int headerHeight = compact ? 44 : 52;
    const int gap = compact ? 8 : 12;
    auto area = getLocalBounds().reduced(20);

    auto header = area.removeFromTop(headerHeight);
    titleLabel.setBounds(header.removeFromLeft(250));
    helpButton.setBounds(header.removeFromRight(70).reduced(0, 6));
    header.removeFromRight(6);
    optionsButton.setBounds(header.removeFromRight(84).reduced(0, 6));
    header.removeFromRight(8);
    buildLabel.setBounds(header.withY(header.getY() + 8).withHeight(24));
    area.removeFromTop(gap);

    auto sourceArea = area.removeFromTop(compact ? 122 : 146)
                           .reduced(14, compact ? 8 : 12);
    sectionSourceLabel.setBounds(sourceArea.removeFromTop(compact ? 18 : 20));
    sourceArea.removeFromTop(compact ? 2 : 4);

    auto loadRow = sourceArea.removeFromBottom(compact ? 30 : 32);
    loadSourceButton.setBounds(loadRow.removeFromRight(150));
    sourceArea.removeFromBottom(4);

    if (compact)
        sourceHintLabel.setBounds({});
    else
        sourceHintLabel.setBounds(sourceArea.removeFromBottom(18));

    sourceDetailsLabel.setBounds(sourceArea.removeFromBottom(compact ? 16 : 18));
    sourceNameLabel.setBounds(sourceArea.removeFromTop(compact ? 22 : 24));
    area.removeFromTop(gap);

    auto modeArea = area.removeFromTop(compact ? 74 : 90).reduced(14, compact ? 8 : 16);
    modeLabel.setBounds(modeArea.removeFromTop(compact ? 16 : 20));
    auto modeRow = modeArea.removeFromTop(compact ? 32 : 38);
    snareButton.setBounds(modeRow.removeFromLeft(132));
    modeRow.removeFromLeft(8);
    clapButton.setBounds(modeRow.removeFromLeft(132));
    area.removeFromTop(gap);

    auto controlsArea = area.removeFromTop(compact ? 148 : 226)
                             .reduced(14, compact ? 8 : 16);
    characterLabel.setBounds(controlsArea.removeFromTop(compact ? 16 : 20));
    sourceCharacterSlider.setBounds(
        controlsArea.removeFromTop(compact ? 28 : 34).removeFromLeft(420));
    controlsArea.removeFromTop(compact ? 4 : 10);

    const auto macroWidth = controlsArea.getWidth() / macroCount;
    for (int i = 0; i < macroCount; ++i)
    {
        auto column = controlsArea.removeFromLeft(macroWidth);
        macroLabels[static_cast<size_t>(i)].setBounds(column.removeFromTop(compact ? 16 : 18));
        macroSliders[static_cast<size_t>(i)].setBounds(column.reduced(4, 0));
    }

    area.removeFromTop(gap);
    auto actionRow = area.removeFromTop(compact ? 34 : 40);
    previewButton.setBounds(actionRow.removeFromLeft(compact ? 96 : 110));
    actionRow.removeFromLeft(6);
    randomizeButton.setBounds(actionRow.removeFromLeft(compact ? 96 : 110));
    actionRow.removeFromLeft(6);
    mutateButton.setBounds(actionRow.removeFromLeft(compact ? 78 : 90));
    actionRow.removeFromLeft(6);
    undoButton.setBounds(actionRow.removeFromLeft(compact ? 68 : 78));
    actionRow.removeFromLeft(6);
    resetButton.setBounds(actionRow.removeFromLeft(compact ? 72 : 84));
    exportButton.setBounds(actionRow.removeFromRight(compact ? 112 : 124));

    area.removeFromTop(compact ? 5 : 8);
    statusLabel.setBounds(area.removeFromTop(compact ? 18 : 24));

    if (helpText.isVisible())
    {
        auto helpArea = getLocalBounds().reduced(55, 45);
        auto closeRow = helpArea.removeFromBottom(40);
        closeHelpButton.setBounds(closeRow.withSizeKeepingCentre(120, 30));
        helpText.setBounds(helpArea);
        helpText.toFront(false);
        closeHelpButton.toFront(false);
    }
}

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    return !files.isEmpty();
}

void MainComponent::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.size() != 1)
    {
        statusLabel.setText("Drop one WAV or AIFF file at a time.", juce::dontSendNotification);
        return;
    }

    loadSourceMetadata(juce::File(files[0]));
}

void MainComponent::chooseSource()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose a WAV or AIFF source",
        juce::File{},
        "*.wav;*.aif;*.aiff");

    const juce::Component::SafePointer<MainComponent> safeThis(this);
    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        {
            const auto selected = chooser.getResult();
            if (safeThis == nullptr)
                return;

            safeThis->fileChooser.reset();
            if (selected.existsAsFile())
                safeThis->loadSourceMetadata(selected);
        });
}

bool MainComponent::hasSupportedExtension(const juce::File& file) const
{
    const auto extension = file.getFileExtension().toLowerCase();
    return extension == ".wav" || extension == ".aif" || extension == ".aiff";
}

void MainComponent::loadSourceMetadata(const juce::File& file)
{
    if (!hasSupportedExtension(file))
    {
        statusLabel.setText("Unsupported file. Choose a WAV or AIFF source.",
                            juce::dontSendNotification);
        return;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
    {
        statusLabel.setText("Could not read that audio file. The previous source is unchanged.",
                            juce::dontSendNotification);
        return;
    }

    currentSource = file;
    const auto durationSeconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
    const auto details = juce::String(reader->numChannels) + " ch  •  "
                       + juce::String(static_cast<int>(reader->sampleRate)) + " Hz  •  "
                       + juce::String(durationSeconds, 2) + " s";

    sourceNameLabel.setText(file.getFileName(), juce::dontSendNotification);
    sourceDetailsLabel.setText(details, juce::dontSendNotification);
    sourceHintLabel.setText("File header validated. Full decode and waveform are next.",
                            juce::dontSendNotification);
    statusLabel.setText("Source selected. Generation and playback are not available in this build.",
                        juce::dontSendNotification);
}

void MainComponent::setMode(bool clapSelected)
{
    clapMode = clapSelected;
    snareButton.setToggleState(!clapMode, juce::dontSendNotification);
    clapButton.setToggleState(clapMode, juce::dontSendNotification);
    statusLabel.setText(clapMode ? "Clap mode selected." : "Snare mode selected.",
                        juce::dontSendNotification);
}
