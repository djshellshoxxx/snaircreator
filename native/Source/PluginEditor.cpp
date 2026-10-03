#include "PluginEditor.h"

namespace {
constexpr auto bg = 0xff090c11;
constexpr auto panel = 0xff111722;
constexpr auto line = 0xff243246;
constexpr auto cyan = 0xff4de4ff;
constexpr auto text = 0xffe8f1ff;
constexpr auto muted = 0xff8ea3bd;
}

SnairCreatorAudioProcessorEditor::SnairCreatorAudioProcessorEditor(SnairCreatorAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setResizable(true, true); setResizeLimits(720, 500, 1600, 1100); setSize(980, 680);
    title.setText("SnairCreator", juce::dontSendNotification); title.setFont(juce::Font(34.0f, juce::Font::bold)); title.setColour(juce::Label::textColourId, juce::Colour(text)); addAndMakeVisible(title);
    subtitle.setText("Circuit Drift Labs  •  source-driven snare / clap synthesis", juce::dontSendNotification); subtitle.setColour(juce::Label::textColourId, juce::Colour(cyan)); addAndMakeVisible(subtitle);
    sourceInfo.setText(processor.getSourceDescription(), juce::dontSendNotification); sourceInfo.setColour(juce::Label::textColourId, juce::Colour(muted)); addAndMakeVisible(sourceInfo);

    mode.addItemList({"Snare","Clap","Hybrid"}, 1); addAndMakeVisible(mode);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, "mode", mode);
    addAndMakeVisible(normalize); normalize.setColour(juce::ToggleButton::textColourId, juce::Colour(text));
    normalizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "normalize", normalize);

    const std::pair<const char*, const char*> defs[] = {
        {"Character","character"},{"Body","body"},{"Body Hz","body_freq_hz"},{"Crack","crack"},{"Noise","noise"},
        {"Tail ms","tail_ms"},{"Clap count","clap_count"},{"Clap spread","clap_spread_ms"},{"Width","width"},
        {"Drive dB","drive_db"},{"Tone","tone"},{"Pitch st","pitch_st"},{"Trim dB","trim_db"},{"Output ms","output_ms"}
    };
    for (auto& d : defs) addControl(d.first, d.second);

    for (auto* b : {&load,&preview,&randomize,&mutate,&reset,&exportWav}) { addAndMakeVisible(*b); b->setColour(juce::TextButton::buttonColourId, juce::Colour(panel)); b->setColour(juce::TextButton::textColourOffId, juce::Colour(text)); }
    load.onClick = [this]{ chooseSource(); }; preview.onClick = [this]{ processor.triggerPreview(); };
    randomize.onClick = [this]{ randomizeParameters(false); }; mutate.onClick = [this]{ randomizeParameters(true); };
    reset.onClick = [this]{ resetParameters(); }; exportWav.onClick = [this]{ chooseExport(); };
    startTimerHz(8);
}

void SnairCreatorAudioProcessorEditor::addControl(const juce::String& name, const juce::String& id) {
    auto c = std::make_unique<Control>();
    c->label.setText(name, juce::dontSendNotification); c->label.setColour(juce::Label::textColourId, juce::Colour(muted));
    c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    c->slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(cyan)); c->slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(text));
    c->slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(line));
    addAndMakeVisible(c->label); addAndMakeVisible(c->slider);
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, id, c->slider);
    controls.push_back(std::move(c));
}

void SnairCreatorAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(bg));
    auto r = getLocalBounds().reduced(18).toFloat();
    g.setColour(juce::Colour(panel)); g.fillRoundedRectangle(r.withTrimmedTop(105), 16.0f);
    g.setColour(juce::Colour(line)); g.drawRoundedRectangle(r.withTrimmedTop(105), 16.0f, 1.0f);
}

void SnairCreatorAudioProcessorEditor::resized() {
    auto r = getLocalBounds().reduced(22);
    title.setBounds(r.removeFromTop(44)); subtitle.setBounds(r.removeFromTop(26));
    auto sourceRow = r.removeFromTop(54).reduced(0, 6); load.setBounds(sourceRow.removeFromLeft(120)); sourceRow.removeFromLeft(10); sourceInfo.setBounds(sourceRow);
    auto modeRow = r.removeFromTop(44).reduced(8, 4); mode.setBounds(modeRow.removeFromLeft(170)); normalize.setBounds(modeRow.removeFromLeft(120));
    r.removeFromTop(8);
    const int cols = getWidth() < 900 ? 4 : 7;
    const int rows = static_cast<int>((controls.size() + cols - 1) / cols);
    auto controlsArea = r.removeFromTop(std::min(r.getHeight() - 58, rows * 145));
    const int w = controlsArea.getWidth() / cols;
    const int h = std::max(120, controlsArea.getHeight() / rows);
    for (size_t i = 0; i < controls.size(); ++i) {
        const int col = static_cast<int>(i) % cols, row = static_cast<int>(i) / cols;
        auto cell = juce::Rectangle<int>(controlsArea.getX() + col*w, controlsArea.getY() + row*h, w, h).reduced(5);
        controls[i]->label.setBounds(cell.removeFromTop(22)); controls[i]->slider.setBounds(cell);
    }
    auto actions = r.removeFromBottom(48); const int bw = std::max(90, actions.getWidth() / 6 - 7);
    for (auto* b : {&preview,&randomize,&mutate,&reset,&exportWav}) { b->setBounds(actions.removeFromLeft(bw)); actions.removeFromLeft(7); }
}

void SnairCreatorAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int) {
    if (files.isEmpty()) return;
    if (!processor.loadSourceFile(juce::File(files[0]))) sourceInfo.setText("Could not decode dropped file", juce::dontSendNotification);
}

void SnairCreatorAudioProcessorEditor::chooseSource() {
    chooser = std::make_unique<juce::FileChooser>("Load source audio", juce::File{}, "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.m4a");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc){
        auto f = fc.getResult(); if (f.existsAsFile()) processor.loadSourceFile(f);
    });
}

void SnairCreatorAudioProcessorEditor::chooseExport() {
    chooser = std::make_unique<juce::FileChooser>("Export generated hit", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("snaircreator.wav"), "*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc){
        auto f = fc.getResult(); if (f.getFileExtension().isEmpty()) f = f.withFileExtension("wav"); processor.exportRendered(f, 24);
    });
}

void SnairCreatorAudioProcessorEditor::randomizeParameters(bool subtle) {
    juce::Random r;
    auto set = [this](const char* id, float actual){ if (auto* p = processor.apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(actual)); };
    if (auto* p = processor.apvts.getParameter("seed")) p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(r.nextInt(0x7fffffff))));
    const float amount = subtle ? .18f : 1.0f;
    auto around = [&](float center, float span){ return center + (r.nextFloat() * 2 - 1) * span * amount; };
    set("body", around(.65f,.3f)); set("crack", around(.7f,.3f)); set("noise", around(.6f,.35f)); set("tail_ms", around(300,260));
    set("clap_spread_ms", around(18,10)); set("drive_db", around(5,6)); set("tone", around(0,.8f)); set("pitch_st", around(0,10));
}

void SnairCreatorAudioProcessorEditor::resetParameters() {
    for (auto* p : processor.getParameters()) if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p)) ranged->setValueNotifyingHost(ranged->getDefaultValue());
}

void SnairCreatorAudioProcessorEditor::timerCallback() { sourceInfo.setText(processor.getSourceDescription(), juce::dontSendNotification); }
