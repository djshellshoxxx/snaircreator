#include "PluginEditor.h"
#include "FactoryPresets.h"
#include "PresetManager.h"

namespace
{
const juce::Colour bg { 0xff0e1116 }, panel { 0xff171b22 }, border { 0xff2a303a }, cyan { 0xff2fcae0 },
                   magenta { 0xffe14ab8 }, text { 0xffe6e8ec }, muted { 0xff8a929e }, good { 0xff7bc96f },
                   warn { 0xfff2c14e }, bad { 0xffef5350 };

const char* helpBody =
    "SNAIRCREATOR " SNAIR_VERSION_STRING " BETA  -  HELP\n\n"
    "WHAT IT DOES\n"
    "SnairCreator turns any WAV or AIFF sound into a playable snare or clap. It analyzes the source, extracts attack, body, "
    "texture and tail material and rebuilds them into a percussion hit. Everything runs locally; there is no network access or telemetry.\n\n"
    "QUICK START\n"
    "1. Drag one WAV/AIFF file onto the window or press LOAD SOURCE.\n"
    "2. Pick SNARE or CLAP. A hit renders automatically in the background.\n"
    "3. Shape it with the six macros and SOURCE CHARACTER. Press PREVIEW (or Space) or play any MIDI note.\n"
    "4. Use RANDOMIZE for a new bounded variation, MUTATE for a small one, UNDO to step back (16 levels), RESET for defaults.\n"
    "5. EXPORT WAV writes the exact sound you hear. EXPORT KIT writes 8 coherent variations to a folder.\n"
    "6. Click the waveform (or SHOW HIT) to view the generated hit, then drag it straight into your DAW or file browser.\n\n"
    "CONTROLS\n"
    "SOURCE CHARACTER - low = stronger conventional snare/clap shaping and procedural reinforcement; high = more of the original source "
    "transient, spectrum and grain texture survives. It changes several internal stages, not a dry/wet mix.\n"
    "PUNCH - impact: attack gain, transient curve, body push, short saturation.\n"
    "SNAP - upper transient: crack filter, click, wire/clap brightness.\n"
    "BODY - low-mid weight: shell resonance level and decay, source body.\n"
    "TEXTURE - source-derived noise: snare wires, clap texture and tail grains.\n"
    "DIRT - saturation, asymmetry and (above 60%) bit crunch.\n"
    "SIZE - decay of body and tail, clap diffusion, room size.\n"
    "GATED ROOM - classic gated reverb burst baked into the render. KEY TRACK - plays the hit chromatically from MIDI (C3/60 = original pitch).\n"
    "ADVANCED - body frequency (Hz), attack window, noise, tail (ms), pitch (semitones), tone tilt, drive (dB), stereo width, clap count and "
    "spread (Clap mode only), cross blend (adds clap bursts to a snare or snare body to a clap), output trim (dB, applied at playback), seed and "
    "render normalization, plus WAV export format options.\n\n"
    "GESTURES\n"
    "Drag knobs to change values; Shift+wheel for very fine steps. Double-click resets to default. Right-click for Enter value / Reset. "
    "Keys: Space preview, R randomize, M mutate, Ctrl/Cmd+Z undo.\n\n"
    "MIDI\n"
    "Any note triggers the current hit (C1/36 and D1/38 are handy defaults). Velocity scales level. Note-off never cuts the one-shot. "
    "Up to 16 overlapping voices; the oldest is reused beyond that. All-notes-off / all-sound-off stop playback.\n\n"
    "PRESETS & SESSIONS\n"
    "Factory presets are parameter recipes. SAVE / SAVE AS / LOAD use .snairpreset files that store settings plus a reference to the source path "
    "(never the audio). Plug-in sessions and the standalone app embed the rendered hit, so your sound comes back even if the source file moved. "
    "When the source is missing the hit stays playable/exportable; load the source again to regenerate.\n\n"
    "FILES\n"
    "Input: WAV, AIF, AIFF, mono or stereo, up to 10 minutes / 256 MB decoded. Silent files are rejected. Export: 16/24-bit PCM or 32-bit float, "
    "mono/stereo, current rate or 44.1/48/88.2/96 kHz. Exports are written to a temporary file, validated, then moved into place.\n\n"
    "OPTIONS > Show tooltips toggles all hover help. Circuit Drift Labs: https://djshellshoxxx.github.io/circuitdriftlabs/\n";
}

//==============================================================================
SnairLookAndFeel::SnairLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, bg);
    setColour(juce::Label::textColourId, text);
    setColour(juce::TextButton::buttonColourId, panel);
    setColour(juce::TextButton::buttonOnColourId, cyan.withAlpha(0.25f));
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::TextButton::textColourOnId, cyan);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::outlineColourId, border);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, cyan.withAlpha(0.3f));
    setColour(juce::Slider::trackColourId, cyan);
    setColour(juce::Slider::backgroundColourId, border);
    setColour(juce::Slider::thumbColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, border);
    setColour(juce::Slider::textBoxBackgroundColourId, bg);
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::rotarySliderFillColourId, cyan);
    setColour(juce::ToggleButton::tickColourId, cyan);
    setColour(juce::TextEditor::backgroundColourId, panel);
    setColour(juce::TextEditor::outlineColourId, cyan);
    setColour(juce::TooltipWindow::backgroundColourId, panel.brighter(0.15f));
    setColour(juce::TooltipWindow::textColourId, text);
}

void SnairLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto area = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)).reduced(4.0f);
    const float r = std::min(area.getWidth(), area.getHeight()) * 0.5f;
    const auto c = area.getCentre();
    const float angle = start + pos * (end - start);
    juce::Path track, value;
    track.addCentredArc(c.x, c.y, r - 3, r - 3, 0, start, end, true);
    value.addCentredArc(c.x, c.y, r - 3, r - 3, 0, start, angle, true);
    g.setColour(border);
    g.strokePath(track, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(s.isEnabled() ? cyan : muted);
    g.strokePath(value, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(panel.brighter(0.1f));
    g.fillEllipse(c.x - r + 9, c.y - r + 9, 2 * (r - 9), 2 * (r - 9));
    g.setColour(magenta);
    const juce::Point<float> tip(c.x + (r - 11) * std::sin(angle), c.y - (r - 11) * std::cos(angle));
    g.drawLine({ c, tip }, 2.5f);
    if (s.hasKeyboardFocus(true)) { g.setColour(cyan.withAlpha(0.6f)); g.drawEllipse(area, 1.0f); }
}

void SnairLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    auto fill = b.getToggleState() ? cyan.withAlpha(0.22f) : base;
    if (!b.isEnabled()) fill = fill.withAlpha(0.4f);
    else if (down) fill = fill.brighter(0.25f);
    else if (over) fill = fill.brighter(0.1f);
    g.setColour(fill);
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(b.hasKeyboardFocus(true) ? cyan : (b.getToggleState() ? cyan.withAlpha(0.8f) : border));
    g.drawRoundedRectangle(r, 5.0f, 1.0f);
}

//==============================================================================
void FineSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (!isEnabled() || !isScrollWheelEnabled()) return;
    const float dir = (std::abs(w.deltaY) > 1.0e-6f ? w.deltaY : -w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
    if (std::abs(dir) < 1.0e-6f) return;
    const auto range = getRange();
    double step = range.getLength() * (e.mods.isShiftDown() ? 0.001 : 0.01);
    if (getInterval() > 0.0) step = std::max(step, getInterval());
    setValue(getValue() + (dir > 0 ? step : -step), juce::sendNotificationSync);
}

void FineSlider::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isPopupMenu()) { juce::Slider::mouseDown(e); return; }
    juce::PopupMenu m;
    m.addItem(1, "Enter value...");
    m.addItem(2, "Reset to default", isDoubleClickReturnEnabled());
    juce::Component::SafePointer<FineSlider> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safe](int r)
    {
        if (safe == nullptr) return;
        if (r == 1) safe->showTextBox();
        if (r == 2) safe->setValue(safe->getDoubleClickReturnValue(), juce::sendNotificationSync);
    });
}

//==============================================================================
SnairCreatorEditor::SnairCreatorEditor(SnairCreatorProcessor& p) : AudioProcessorEditor(p), proc(p)
{
    setLookAndFeel(&lnf);
    auto label = [this](juce::Label& l, const juce::String& t, float size, juce::Colour c, bool bold = false)
    {
        l.setText(t, juce::dontSendNotification);
        l.setFont(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
        l.setColour(juce::Label::textColourId, c);
        addAndMakeVisible(l);
    };
    label(title, "SnairCreator", 24.0f, text, true);
    label(version, "v" SNAIR_VERSION_STRING " BETA  /  CIRCUIT DRIFT LABS", 11.0f, cyan, true);
    version.setMinimumHorizontalScale(0.6f);
    label(sourceHeading, "SOURCE AUDIO", 11.0f, cyan, true);
    label(sourceName, "No source loaded", 15.0f, text, true);
    label(sourceDetails, "WAV or AIFF  -  one file", 11.5f, muted);
    label(analysisDetails, "", 11.0f, muted);
    analysisDetails.setJustificationType(juce::Justification::topLeft);
    analysisDetails.setMinimumHorizontalScale(0.8f);
    label(statusLabel, "", 12.0f, muted);
    statusLabel.setMinimumHorizontalScale(0.7f);
    label(midiLabel, "MIDI o", 12.0f, muted, true);
    midiLabel.setTooltip("MIDI activity: filled dot flashes on note-on; shows active voices.");
    label(charLabel, "SOURCE CHARACTER", 11.0f, muted, true);
    label(roomLabel, "GATED ROOM", 11.0f, muted, true);
    label(exportHeading, "EXPORT", 11.0f, cyan, true);

    addAndMakeVisible(waveform);
    waveform.setTooltip("Click to switch between source and generated hit. Drag the generated hit into your DAW or desktop.");
    waveform.onClicked = [this] { waveform.setShowHit(!waveform.isShowingHit()); refresh(); };
    waveform.onDragOut = [this] { dragHitOut(); };

    setupButton(loadSourceButton, "Choose a WAV or AIFF source file (or drop one onto the window). Loading again relinks a missing source.");
    loadSourceButton.onClick = [this] { chooseSource(); };
    setupButton(viewButton, "Toggle the waveform between the source and the generated hit.");
    viewButton.onClick = [this] { waveform.setShowHit(!waveform.isShowingHit()); refresh(); };

    setupButton(snareButton, "Generate a snare: attack crack + shell body + wire texture.");
    setupButton(clapButton, "Generate a clap: 2-6 source-derived micro-bursts + texture tail.");
    snareButton.setClickingTogglesState(false);
    snareButton.onClick = [this] { auto q = proc.currentParams(); q.mode = SnairMode::snare; proc.applyParameters(q); refresh(); };
    clapButton.onClick = [this] { auto q = proc.currentParams(); q.mode = SnairMode::clap; proc.applyParameters(q); refresh(); };

    setupSlider(sourceCharacter, ParamIDs::sourceCharacter, "How much of the source identity survives (multi-stage, not dry/wet).", false);
    setupSlider(roomSlider, ParamIDs::room, "Gated room: an 80s-style gated reverb burst rendered into the hit. Size lengthens the gate.", false);
    keyTrackToggle.setTooltip("Key Track: MIDI notes repitch the hit chromatically (C3/60 = original). Off = every note plays the same hit.");
    addAndMakeVisible(keyTrackToggle);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(proc.state(), ParamIDs::keyTrack, keyTrackToggle));

    const char* macroIds[macroCount] = { ParamIDs::punch, ParamIDs::snap, ParamIDs::body, ParamIDs::texture, ParamIDs::dirt, ParamIDs::size };
    const char* macroNames[macroCount] = { "PUNCH", "SNAP", "BODY", "TEXTURE", "DIRT", "SIZE" };
    const char* macroTips[macroCount] = {
        "Punch: perceived impact - attack gain, transient curve, body push and short saturation.",
        "Snap: upper transient - crack filter, click and wire/clap brightness.",
        "Body: low-mid drum weight - shell resonance level/decay and source body.",
        "Texture: source-derived noise - snare wires, clap texture and tail grains.",
        "Dirt: nonlinear character - saturation, asymmetry and bit crunch above 60%.",
        "Size: perceived size - body and tail decay, clap diffusion and room size." };
    for (int i = 0; i < macroCount; ++i)
    {
        label(macroLabels[static_cast<size_t>(i)], macroNames[i], 11.0f, muted, true);
        macroLabels[static_cast<size_t>(i)].setJustificationType(juce::Justification::centred);
        setupSlider(macros[static_cast<size_t>(i)], macroIds[i], macroTips[i], true);
    }

    const char* advIds[advCount] = { ParamIDs::bodyFreqHz, ParamIDs::attack, ParamIDs::noise, ParamIDs::tailMs, ParamIDs::pitchSt,
                                     ParamIDs::tone, ParamIDs::driveDb, ParamIDs::width, ParamIDs::clapCount, ParamIDs::clapSpreadMs,
                                     ParamIDs::crossBlend, ParamIDs::outputTrimDb, ParamIDs::seed };
    const char* advNames[advCount] = { "BODY FREQ", "ATTACK", "NOISE", "TAIL", "PITCH", "TONE", "DRIVE", "WIDTH",
                                       "CLAP COUNT", "CLAP SPREAD", "CROSS BLEND", "OUTPUT TRIM", "SEED" };
    const char* advTips[advCount] = {
        "Body resonance frequency, 70-450 Hz (blended with the analyzed source body).",
        "Attack window length (about 5-40 ms) and clap burst length.",
        "Noise/wire amount relative to the rest of the hit.",
        "Tail length, 20-2000 ms.",
        "Pitch shift of source material, -24 to +24 semitones.",
        "Tone tilt: negative darker, positive brighter.",
        "Saturation drive, 0-24 dB (output stays peak-safe).",
        "Stereo width: 0 mono, 50% natural, 100% wide.",
        "Clap mode only: number of micro-bursts (2-6).",
        "Clap mode only: spacing between bursts (8-35 ms).",
        "Adds clap bursts to a snare, or snare body to a clap.",
        "Playback/export level trim, -24 to +12 dB. Applied after the render; no re-render needed.",
        "Deterministic seed. Same source + settings + seed = same hit. Type a number to recall one." };
    for (int i = 0; i < advCount; ++i)
    {
        auto& l = advLabels[static_cast<size_t>(i)];
        l.setText(advNames[i], juce::dontSendNotification);
        l.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, muted);
        addChildComponent(l);
        auto& s = adv[static_cast<size_t>(i)];
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
        s.setTooltip(advTips[i]);
        addChildComponent(s);
        sliderAttachments.push_back(std::make_unique<SliderAttachment>(proc.state(), advIds[i], s));
        if (auto* param = proc.state().getParameter(advIds[i]))
            s.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
    }
    adv[12].setNumDecimalPlacesToDisplay(0);
    normalizeToggle.setTooltip("Normalize the rendered hit to a 0.98 peak before output trim.");
    addChildComponent(normalizeToggle);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(proc.state(), ParamIDs::normalizeRender, normalizeToggle));

    bitDepthBox.addItem("16-bit PCM", 16); bitDepthBox.addItem("24-bit PCM", 24); bitDepthBox.addItem("32-bit float", 32);
    rateBox.addItem("Render rate", 1); rateBox.addItem("44.1 kHz", 2); rateBox.addItem("48 kHz", 3); rateBox.addItem("88.2 kHz", 4); rateBox.addItem("96 kHz", 5);
    channelsBox.addItem("Stereo", 2); channelsBox.addItem("Mono", 1);
    bitDepthBox.setTooltip("WAV sample format for Export WAV, Export Kit and drag-out.");
    rateBox.setTooltip("Export sample rate. Different rates use high-quality offline resampling.");
    channelsBox.setTooltip("Export channel layout. Mono sums left and right.");
    exportNormalize.setTooltip("Normalize the exported file to a 0.98 peak.");
    exportTrim.setTooltip("Apply the Output Trim value to exported files.");
    static const double rates[] = { 0.0, 44100.0, 48000.0, 88200.0, 96000.0 };
    auto syncExport = [this]
    {
        proc.exportOptions.bitDepth = bitDepthBox.getSelectedId();
        proc.exportOptions.sampleRate = rates[juce::jlimit(1, 5, rateBox.getSelectedId()) - 1];
        proc.exportOptions.channels = channelsBox.getSelectedId();
        proc.exportOptions.normalize = exportNormalize.getToggleState();
        proc.exportOptions.includeOutputTrim = exportTrim.getToggleState();
    };
    for (auto* c : std::initializer_list<juce::Component*> { &bitDepthBox, &rateBox, &channelsBox, &exportNormalize, &exportTrim, &exportHeading })
        addChildComponent(c);
    bitDepthBox.setSelectedId(proc.exportOptions.bitDepth == 16 || proc.exportOptions.bitDepth == 32 ? proc.exportOptions.bitDepth : 24, juce::dontSendNotification);
    int rateId = 1;
    for (int i = 1; i < 5; ++i) if (std::abs(proc.exportOptions.sampleRate - rates[i]) < 1.0) rateId = i + 1;
    rateBox.setSelectedId(rateId, juce::dontSendNotification);
    channelsBox.setSelectedId(proc.exportOptions.channels == 1 ? 1 : 2, juce::dontSendNotification);
    exportNormalize.setToggleState(proc.exportOptions.normalize, juce::dontSendNotification);
    exportTrim.setToggleState(proc.exportOptions.includeOutputTrim, juce::dontSendNotification);
    bitDepthBox.onChange = rateBox.onChange = channelsBox.onChange = syncExport;
    exportNormalize.onClick = exportTrim.onClick = syncExport;

    setupButton(previewButton, "Play the active hit (same buffer as MIDI and export). Shortcut: Space.");
    setupButton(randomizeButton, "New, substantially different but bounded variation (mode is kept). Shortcut: R.");
    setupButton(mutateButton, "Small variation (about 15% moves) that keeps the sound's identity. Shortcut: M.");
    setupButton(undoButton, "Undo the last Randomize/Mutate/Reset/preset change (16 levels). Shortcut: Ctrl/Cmd+Z.");
    setupButton(resetButton, "Reset all parameters to defaults (keeps the source and mode; undoable).");
    setupButton(advancedButton, "Show/hide the detailed parameter and export panel.");
    setupButton(exportButton, "Write the active hit to a WAV file using the export options.");
    setupButton(kitButton, "Render the current sound plus 7 mutations as a ready-to-use sample kit folder.");
    previewButton.onClick = [this] { proc.triggerPreview(); };
    randomizeButton.onClick = [this] { proc.randomize(); refresh(); };
    mutateButton.onClick = [this] { proc.mutate(); refresh(); };
    undoButton.onClick = [this] { proc.undo(); refresh(); };
    resetButton.onClick = [this] { proc.resetParameters(); refresh(); };
    advancedButton.onClick = [this] { advancedVisible = !advancedVisible; advancedButton.setToggleState(advancedVisible, juce::dontSendNotification); resized(); repaint(); };
    exportButton.onClick = [this] { exportWav(); };
    kitButton.onClick = [this] { exportKit(); };

    setupButton(savePresetButton, "Save over the current user preset (or choose a file).");
    setupButton(saveAsButton, "Save the current settings as a new .snairpreset file.");
    setupButton(loadPresetButton, "Load a .snairpreset file (undoable; the source is not replaced).");
    setupButton(optionsButton, "Interface options, including the global tooltip switch.");
    setupButton(helpButton, "Open the complete SnairCreator help.");
    savePresetButton.onClick = [this] { savePreset(false); };
    saveAsButton.onClick = [this] { savePreset(true); };
    loadPresetButton.onClick = [this] { loadPresetFile(); };
    optionsButton.onClick = [this] { showOptions(); };
    helpButton.onClick = [this] { setHelpVisible(true); };
    presetBox.setTooltip("Factory recipes and your saved presets.");
    presetBox.setTextWhenNothingSelected("Presets");
    presetBox.onChange = [this] { presetChosen(); };
    addAndMakeVisible(presetBox);
    rebuildPresetList();

    helpText.setMultiLine(true);
    helpText.setReadOnly(true);
    helpText.setScrollbarsShown(true);
    helpText.setCaretVisible(false);
    helpText.setFont(juce::FontOptions(13.5f));
    helpText.setColour(juce::TextEditor::textColourId, text);
    helpText.setText(helpBody);
    helpBackdrop.setColour(juce::Label::backgroundColourId, bg.withAlpha(0.94f));
    helpBackdrop.setInterceptsMouseClicks(true, false);
    addChildComponent(helpBackdrop);
    addChildComponent(helpText);
    addChildComponent(closeHelpButton);
    closeHelpButton.onClick = [this] { setHelpVisible(false); };

    applyTooltips();
    setWantsKeyboardFocus(true);
    setResizable(true, true);
    setResizeLimits(760, 520, 2400, 1600);
    setSize(1000, 700);
    proc.addChangeListener(this);
    startTimerHz(15);
    refresh();
}

SnairCreatorEditor::~SnairCreatorEditor()
{
    proc.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void SnairCreatorEditor::setupSlider(FineSlider& s, const char* id, const juce::String& tip, bool rotary)
{
    s.setSliderStyle(rotary ? juce::Slider::RotaryHorizontalVerticalDrag : juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(rotary ? juce::Slider::TextBoxBelow : juce::Slider::TextBoxRight, false, rotary ? 64 : 56, 20);
    s.setTooltip(tip);
    addAndMakeVisible(s);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(proc.state(), id, s));
    if (auto* param = proc.state().getParameter(id))
        s.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
}

void SnairCreatorEditor::setupButton(juce::Button& b, const juce::String& tip)
{
    b.setTooltip(tip);
    addAndMakeVisible(b);
}

void SnairCreatorEditor::applyTooltips()
{
    if (proc.tooltipsEnabled && tooltipWindow == nullptr) tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 700);
    if (!proc.tooltipsEnabled) tooltipWindow.reset();
}

void SnairCreatorEditor::showOptions()
{
    juce::PopupMenu m;
    m.addSectionHeader("Interface");
    m.addItem(1, "Show tooltips", true, proc.tooltipsEnabled);
    m.addItem(2, "Open user preset folder");
    juce::Component::SafePointer<SnairCreatorEditor> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&optionsButton), [safe](int r)
    {
        if (safe == nullptr) return;
        if (r == 1) { safe->proc.tooltipsEnabled = !safe->proc.tooltipsEnabled; safe->applyTooltips(); }
        if (r == 2) { PresetManager::userFolder().createDirectory(); PresetManager::userFolder().startAsProcess(); }
    });
}

void SnairCreatorEditor::setHelpVisible(bool v)
{
    helpBackdrop.setVisible(v);
    helpText.setVisible(v);
    closeHelpButton.setVisible(v);
    resized();
    if (v) { helpBackdrop.toFront(false); helpText.toFront(true); closeHelpButton.toFront(false); }
}

//==============================================================================
void SnairCreatorEditor::timerCallback()
{
    const auto m = proc.midiActivityCount();
    if (m != lastMidi) { lastMidi = m; midiFlash = 3; }
    const int voices = proc.activeVoices();
    midiLabel.setText(juce::String(midiFlash > 0 ? "MIDI *" : "MIDI o") + "  " + juce::String(voices) + "/16", juce::dontSendNotification);
    midiLabel.setColour(juce::Label::textColourId, midiFlash > 0 ? magenta : muted);
    if (midiFlash > 0) --midiFlash;
    if (transientTicks > 0 && --transientTicks == 0) refresh();
    // Clap-only controls follow the mode, including host automation of the mode parameter.
    const bool clap = proc.currentParams().mode == SnairMode::clap;
    if (adv[8].isEnabled() != clap) refresh();
}

void SnairCreatorEditor::refresh()
{
    const auto p = proc.currentParams();
    const bool clap = p.mode == SnairMode::clap;
    snareButton.setToggleState(!clap, juce::dontSendNotification);
    clapButton.setToggleState(clap, juce::dontSendNotification);
    adv[8].setEnabled(clap); adv[9].setEnabled(clap);
    advLabels[8].setAlpha(clap ? 1.0f : 0.45f); advLabels[9].setAlpha(clap ? 1.0f : 0.45f);

    const auto src = proc.currentSource();
    const auto hit = proc.currentHit();
    const auto* a = proc.currentAnalysis();
    if (src.get() != shownSource)
    {
        shownSource = src.get();
        if (src && a && src->frameCount > 0)
        {
            const float n = static_cast<float>(src->frameCount);
            waveform.setSource(src->waveformPeaks, a->strongestTransientSample / n, a->attackWindowStart / n, a->attackWindowEnd / n);
            sourceName.setText(src->sourceFile.getFileName(), juce::dontSendNotification);
            sourceDetails.setText(juce::String(src->channelCount) + (src->channelCount == 1 ? " ch (mono)" : " ch (stereo, analyzed as L+R mid)")
                                  + "  -  " + juce::String(src->sampleRate / 1000.0, 1) + " kHz  -  " + juce::String(src->durationSeconds(), 2) + " s",
                                  juce::dontSendNotification);
            analysisDetails.setText("Peak " + juce::String(juce::Decibels::gainToDecibels(a->peak), 1) + " dB  crest " + juce::String(a->crest, 1)
                                  + "  centroid " + juce::String(juce::roundToInt(a->spectralCentroidHz)) + " Hz\nbody " + juce::String(juce::roundToInt(a->dominantBodyHz))
                                  + " Hz  " + juce::String(static_cast<int>(a->transientCandidates.size())) + " transients  "
                                  + (a->noisyEstimate > 0.5f ? "noisy" : "tonal"), juce::dontSendNotification);
        }
        else if (proc.isSourceMissing() || !src)
        {
            waveform.setSource({}, -1, -1, -1);
            sourceName.setText(proc.getSourceName().isNotEmpty() ? proc.getSourceName() : juce::String("No source loaded"), juce::dontSendNotification);
            sourceDetails.setText(proc.isSourceMissing() ? "SOURCE UNAVAILABLE - load/relink to regenerate" : "WAV or AIFF  -  one file", juce::dontSendNotification);
            analysisDetails.setText(proc.isSourceMissing() ? "The saved hit is still playable." : "", juce::dontSendNotification);
            waveform.setMissingSourceNotice(proc.isSourceMissing() ? "ORIGINAL SOURCE MISSING  -  SAVED HIT IS ACTIVE" : "");
        }
    }
    sourceDetails.setColour(juce::Label::textColourId, proc.isSourceMissing() && !src ? warn : muted);
    if (hit.get() != shownHit) { shownHit = hit.get(); waveform.setHit(hit ? hit->peaks : std::vector<float> {}); if (!src && hit) waveform.setShowHit(true); }
    viewButton.setButtonText(waveform.isShowingHit() ? "SHOW SOURCE" : "SHOW HIT");

    const bool hasHit = hit != nullptr, hasSource = src != nullptr;
    previewButton.setEnabled(hasHit);
    exportButton.setEnabled(hasHit);
    kitButton.setEnabled(hasSource);
    randomizeButton.setEnabled(hasSource || hasHit);
    mutateButton.setEnabled(hasSource || hasHit);
    undoButton.setEnabled(proc.canUndo());

    using S = SnairCreatorProcessor::Status;
    const auto st = proc.getStatus();
    auto msg = transientTicks > 0 ? transientMessage : proc.getStatusText();
    if (transientTicks == 0 && hasHit && st != S::rendering && proc.isHitStale() && hasSource)
        msg << "   [pending: active hit does not match the visible settings]";
    statusLabel.setText(msg, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, st == S::error ? bad : st == S::rendering || st == S::loading ? warn : st == S::ready ? good : muted);
}

//==============================================================================
void SnairCreatorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    auto drawPanel = [&](juce::Rectangle<int> r)
    {
        if (r.isEmpty()) return;
        g.setColour(panel);
        g.fillRoundedRectangle(r.toFloat(), 9.0f);
        g.setColour(border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 9.0f, 1.0f);
    };
    drawPanel(sourcePanel);
    drawPanel(modePanel);
    drawPanel(macroPanel);
}

void SnairCreatorEditor::resized()
{
    auto area = getLocalBounds().reduced(14);
    const bool compact = getHeight() < 620;
    const int gap = compact ? 7 : 10;

    auto header = area.removeFromTop(compact ? 34 : 40);
    helpButton.setBounds(header.removeFromRight(60).reduced(0, 4)); header.removeFromRight(5);
    optionsButton.setBounds(header.removeFromRight(76).reduced(0, 4)); header.removeFromRight(5);
    loadPresetButton.setBounds(header.removeFromRight(56).reduced(0, 4)); header.removeFromRight(5);
    saveAsButton.setBounds(header.removeFromRight(70).reduced(0, 4)); header.removeFromRight(5);
    savePresetButton.setBounds(header.removeFromRight(56).reduced(0, 4)); header.removeFromRight(5);
    presetBox.setBounds(header.removeFromRight(juce::jlimit(150, 220, header.getWidth() - 330)).reduced(0, 4));
    title.setBounds(header.removeFromLeft(160));
    version.setBounds(header);
    area.removeFromTop(gap);

    auto bottom = area.removeFromBottom(24);
    midiLabel.setBounds(bottom.removeFromLeft(110));
    statusLabel.setBounds(bottom);
    area.removeFromBottom(gap / 2);

    auto actions = area.removeFromBottom(compact ? 32 : 38);
    const int bw = juce::jmin(110, (actions.getWidth() - 8 * 6) / 8);
    for (auto* b : { &previewButton, &randomizeButton, &mutateButton, &undoButton, &resetButton, &advancedButton })
    { b->setBounds(actions.removeFromLeft(bw)); actions.removeFromLeft(6); }
    exportButton.setBounds(actions.removeFromRight(bw + 10));
    actions.removeFromRight(6);
    kitButton.setBounds(actions.removeFromRight(bw + 10));
    area.removeFromBottom(gap);

    sourcePanel = area.removeFromTop(juce::jlimit(118, 190, area.getHeight() * 28 / 100));
    {
        auto r = sourcePanel.reduced(12, 8);
        auto left = r.removeFromLeft(juce::jmin(290, r.getWidth() / 3));
        sourceHeading.setBounds(left.removeFromTop(16));
        sourceName.setBounds(left.removeFromTop(22));
        sourceDetails.setBounds(left.removeFromTop(18));
        loadSourceButton.setBounds(left.removeFromBottom(28).removeFromLeft(140));
        left.removeFromBottom(4);
        analysisDetails.setBounds(left);
        r.removeFromLeft(10);
        auto top = r.removeFromTop(22);
        viewButton.setBounds(top.removeFromRight(110).reduced(0, 1));
        r.removeFromTop(4);
        waveform.setBounds(r);
    }
    area.removeFromTop(gap);

    modePanel = area.removeFromTop(compact ? 50 : 56);
    {
        auto r = modePanel.reduced(12, 9);
        const bool narrow = getWidth() < 900; // stack labels above sliders when space is tight
        snareButton.setBounds(r.removeFromLeft(narrow ? 84 : 100)); r.removeFromLeft(6);
        clapButton.setBounds(r.removeFromLeft(narrow ? 84 : 100)); r.removeFromLeft(narrow ? 10 : 18);
        keyTrackToggle.setBounds(r.removeFromRight(100));
        r.removeFromRight(10);
        auto room = r.removeFromRight(r.getWidth() * 2 / 5);
        r.removeFromRight(10);
        auto place = [narrow](juce::Label& l, juce::Slider& sl, juce::Rectangle<int> cell, int labelWidth)
        {
            if (narrow) { l.setBounds(cell.removeFromTop(13)); sl.setBounds(cell); }
            else { l.setBounds(cell.removeFromLeft(labelWidth)); sl.setBounds(cell); }
        };
        place(roomLabel, roomSlider, room, 84);
        place(charLabel, sourceCharacter, r, 128);
    }
    area.removeFromTop(gap);

    macroPanel = area;
    auto inner = macroPanel.reduced(12, 8);
    for (int i = 0; i < macroCount; ++i)
    {
        const bool show = !advancedVisible;
        macros[static_cast<size_t>(i)].setVisible(show);
        macroLabels[static_cast<size_t>(i)].setVisible(show);
    }
    for (int i = 0; i < advCount; ++i) { adv[static_cast<size_t>(i)].setVisible(advancedVisible); advLabels[static_cast<size_t>(i)].setVisible(advancedVisible); }
    for (auto* c : std::initializer_list<juce::Component*> { &normalizeToggle, &bitDepthBox, &rateBox, &channelsBox, &exportNormalize, &exportTrim, &exportHeading })
        c->setVisible(advancedVisible);

    if (!advancedVisible)
    {
        const int w = inner.getWidth() / macroCount;
        for (int i = 0; i < macroCount; ++i)
        {
            auto col = inner.removeFromLeft(w).reduced(4, 0);
            macroLabels[static_cast<size_t>(i)].setBounds(col.removeFromTop(18));
            const int knob = juce::jmin(col.getWidth(), col.getHeight());
            macros[static_cast<size_t>(i)].setBounds(col.withSizeKeepingCentre(col.getWidth(), knob));
        }
    }
    else
    {
        auto exportRow = inner.removeFromBottom(26);
        inner.removeFromBottom(6);
        const int cols = 3, rows = 5;
        const int cw = inner.getWidth() / cols, rh = juce::jmin(34, inner.getHeight() / rows);
        for (int i = 0; i < advCount + 1; ++i)
        {
            auto cell = juce::Rectangle<int>(inner.getX() + (i / rows) * cw, inner.getY() + (i % rows) * rh, cw, rh).reduced(6, 3);
            if (i == advCount) { normalizeToggle.setBounds(cell); continue; }
            advLabels[static_cast<size_t>(i)].setBounds(cell.removeFromLeft(juce::jmin(90, cell.getWidth() / 3)));
            adv[static_cast<size_t>(i)].setBounds(cell);
        }
        exportHeading.setBounds(exportRow.removeFromLeft(60));
        const int ew = exportRow.getWidth() / 5;
        bitDepthBox.setBounds(exportRow.removeFromLeft(ew).reduced(3, 0));
        rateBox.setBounds(exportRow.removeFromLeft(ew).reduced(3, 0));
        channelsBox.setBounds(exportRow.removeFromLeft(ew).reduced(3, 0));
        exportNormalize.setBounds(exportRow.removeFromLeft(ew).reduced(3, 0));
        exportTrim.setBounds(exportRow.reduced(3, 0));
    }

    if (helpText.isVisible())
    {
        helpBackdrop.setBounds(getLocalBounds());
        auto h = getLocalBounds().reduced(40, 30);
        closeHelpButton.setBounds(h.removeFromBottom(34).withSizeKeepingCentre(130, 30));
        helpText.setBounds(h.withTrimmedBottom(6));
    }
}

//==============================================================================
bool SnairCreatorEditor::keyPressed(const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey) { proc.triggerPreview(); return true; }
    if (k.getModifiers().isCommandDown() && k.getKeyCode() == 'Z') { proc.undo(); refresh(); return true; }
    if (k.getModifiers().isAnyModifierKeyDown()) return false;
    const auto c = juce::CharacterFunctions::toLowerCase(k.getTextCharacter());
    if (c == 'r' && randomizeButton.isEnabled()) { proc.randomize(); refresh(); return true; }
    if (c == 'm' && mutateButton.isEnabled()) { proc.mutate(); refresh(); return true; }
    return false;
}

bool SnairCreatorEditor::isInterestedInFileDrag(const juce::StringArray& files) { return files.size() >= 1; }

void SnairCreatorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.size() != 1) { transientMessage = "Drop one WAV or AIFF file at a time."; transientTicks = 45; refresh(); return; }
    const juce::File f(files[0]);
    if (f.hasFileExtension(PresetManager::extension)) { juce::String e, w, sp; SnairParameters q;
        if (PresetManager::load(f, q, e, w, sp)) { proc.loadParametersAsAction(q); currentPresetFile = f; } transientMessage = e.isNotEmpty() ? e : (w.isNotEmpty() ? w : "Preset loaded: " + f.getFileNameWithoutExtension()); transientTicks = 60; refresh(); return; }
    proc.loadSource(f);
}

void SnairCreatorEditor::chooseSource()
{
    chooser = std::make_unique<juce::FileChooser>("Choose a WAV or AIFF source", juce::File {}, "*.wav;*.aif;*.aiff");
    juce::Component::SafePointer<SnairCreatorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        const auto f = fc.getResult();
        if (f.existsAsFile()) safe->proc.loadSource(f);
    });
}

void SnairCreatorEditor::exportWav()
{
    if (!proc.currentHit()) return;
    auto start = juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile(proc.defaultExportName());
    chooser = std::make_unique<juce::FileChooser>("Export WAV", start, "*.wav");
    juce::Component::SafePointer<SnairCreatorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr || fc.getResult() == juce::File {}) return;
        auto f = fc.getResult().withFileExtension(".wav");
        juce::String msg;
        safe->proc.exportHit(f, msg);
        safe->transientMessage = msg; safe->transientTicks = 120; safe->refresh();
    });
}

void SnairCreatorEditor::exportKit()
{
    chooser = std::make_unique<juce::FileChooser>("Choose a folder for the 8-sound kit", juce::File::getSpecialLocation(juce::File::userMusicDirectory));
    juce::Component::SafePointer<SnairCreatorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr || fc.getResult() == juce::File {}) return;
        const auto folder = fc.getResult().getChildFile(juce::File::createLegalFileName(safe->proc.defaultExportName("_Kit")).upToLastOccurrenceOf(".wav", false, true));
        safe->transientMessage = "Rendering kit..."; safe->transientTicks = 600; safe->refresh();
        safe->proc.exportKit(folder, 8, [safe](juce::String m) { if (safe != nullptr) { safe->transientMessage = m; safe->transientTicks = 150; safe->refresh(); } });
    });
}

void SnairCreatorEditor::dragHitOut()
{
    juce::String error;
    const auto f = proc.writeDragFile(error);
    if (f == juce::File {}) { transientMessage = error; transientTicks = 60; refresh(); return; }
    juce::DragAndDropContainer::performExternalDragDropOfFiles({ f.getFullPathName() }, false, this);
    transientMessage = "Dragging " + f.getFileName(); transientTicks = 45; refresh();
}

//==============================================================================
void SnairCreatorEditor::rebuildPresetList()
{
    presetBox.clear(juce::dontSendNotification);
    presetBox.addSectionHeading("Factory");
    const auto& factory = FactoryPresets::all();
    for (int i = 0; i < static_cast<int>(factory.size()); ++i) presetBox.addItem(factory[static_cast<size_t>(i)].name, i + 1);
    userPresets = PresetManager::userFolder().findChildFiles(juce::File::findFiles, false, juce::String("*") + PresetManager::extension);
    userPresets.sort();
    if (!userPresets.isEmpty())
    {
        presetBox.addSectionHeading("User");
        for (int i = 0; i < userPresets.size(); ++i) presetBox.addItem(userPresets[i].getFileNameWithoutExtension(), 1000 + i);
    }
}

void SnairCreatorEditor::presetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0) return;
    const auto& factory = FactoryPresets::all();
    if (id <= static_cast<int>(factory.size()))
    {
        proc.loadParametersAsAction(factory[static_cast<size_t>(id - 1)].parameters);
        currentPresetFile = juce::File();
        transientMessage = "Factory preset: " + factory[static_cast<size_t>(id - 1)].name + " (parameter recipe; uses your loaded source)";
    }
    else if (id >= 1000 && id - 1000 < userPresets.size())
    {
        const auto f = userPresets[id - 1000];
        SnairParameters q; juce::String e, w, sp;
        if (PresetManager::load(f, q, e, w, sp))
        {
            proc.loadParametersAsAction(q);
            currentPresetFile = f;
            transientMessage = "Preset loaded: " + f.getFileNameWithoutExtension() + (w.isNotEmpty() ? "  -  " + w : juce::String())
                             + (sp.isNotEmpty() && !juce::File(sp).existsAsFile() ? "  -  its original source is unavailable" : juce::String());
        }
        else transientMessage = "Preset not loaded: " + e;
    }
    transientTicks = 75;
    refresh();
}

void SnairCreatorEditor::savePreset(bool forceChooser)
{
    const auto src = proc.currentSource();
    const auto srcPath = src ? src->sourceFile.getFullPathName() : juce::String();
    const auto fp = proc.currentAnalysis() ? proc.currentAnalysis()->fingerprint : 0;
    if (!forceChooser && currentPresetFile.existsAsFile())
    {
        juce::String e;
        transientMessage = PresetManager::save(currentPresetFile, proc.currentParams(), srcPath, fp, e) ? "Saved " + currentPresetFile.getFileName() : e;
        transientTicks = 75; refresh();
        return;
    }
    PresetManager::userFolder().createDirectory();
    chooser = std::make_unique<juce::FileChooser>("Save preset", PresetManager::userFolder().getChildFile("My Snair" + juce::String(PresetManager::extension)),
                                                  juce::String("*") + PresetManager::extension);
    juce::Component::SafePointer<SnairCreatorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [safe, srcPath, fp](const juce::FileChooser& fc)
    {
        if (safe == nullptr || fc.getResult() == juce::File {}) return;
        const auto f = fc.getResult().withFileExtension(PresetManager::extension);
        juce::String e;
        if (PresetManager::save(f, safe->proc.currentParams(), srcPath, fp, e)) { safe->currentPresetFile = f; safe->transientMessage = "Saved " + f.getFileName(); }
        else safe->transientMessage = e;
        safe->transientTicks = 75;
        safe->rebuildPresetList();
        safe->refresh();
    });
}

void SnairCreatorEditor::loadPresetFile()
{
    chooser = std::make_unique<juce::FileChooser>("Load preset", PresetManager::userFolder(), juce::String("*") + PresetManager::extension);
    juce::Component::SafePointer<SnairCreatorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr || !fc.getResult().existsAsFile()) return;
        juce::StringArray one; one.add(fc.getResult().getFullPathName());
        safe->filesDropped(one, 0, 0);
    });
}
