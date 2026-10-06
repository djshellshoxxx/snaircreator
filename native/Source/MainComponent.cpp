#include "MainComponent.h"
#include "SourceAnalyzer.h"
#include "SourceLoader.h"
#include "SnairEngine.h"
#include "SessionStore.h"
#include "FactoryPresets.h"
#include "WavExporter.h"

namespace
{
const juce::Colour bg{15,18,23}, panel{24,29,37}, border{48,58,70}, accent{47,202,224}, muted{145,157,171};

juce::String legalStem(juce::String value)
{
    value=value.replaceCharacters("\\\/:*?\"<>|","_________").trim();
    return value.isNotEmpty()?value:"Source";
}
}

MainComponent::MainComponent()
{
    titleLabel.setText("SnairCreator",juce::dontSendNotification);
    titleLabel.setFont(juce::Font(27.0f,juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId,juce::Colours::white);
    addAndMakeVisible(titleLabel);

    buildLabel.setText("STANDALONE  /  COMPLETE ENGINE",juce::dontSendNotification);
    buildLabel.setFont(juce::Font(11.0f,juce::Font::bold));
    buildLabel.setColour(juce::Label::textColourId,accent);
    addAndMakeVisible(buildLabel);
    tooltipWindow=std::make_unique<juce::TooltipWindow>(nullptr,700);

    sectionSourceLabel.setText("SOURCE AUDIO",juce::dontSendNotification);
    sectionSourceLabel.setColour(juce::Label::textColourId,accent);
    addAndMakeVisible(sectionSourceLabel);

    sourceNameLabel.setText("No source loaded",juce::dontSendNotification);
    sourceNameLabel.setColour(juce::Label::textColourId,juce::Colours::white);
    addAndMakeVisible(sourceNameLabel);

    sourceDetailsLabel.setText("WAV or AIFF • one file at a time",juce::dontSendNotification);
    sourceDetailsLabel.setColour(juce::Label::textColourId,muted);
    addAndMakeVisible(sourceDetailsLabel);

    sourceHintLabel.setText("Drop a WAV or AIFF here",juce::dontSendNotification);
    sourceHintLabel.setColour(juce::Label::textColourId,muted);
    addAndMakeVisible(sourceHintLabel);
    addAndMakeVisible(waveform);

    for(auto* b:{&loadSourceButton,&snareButton,&clapButton,&previewButton,&randomizeButton,&mutateButton,&undoButton,&resetButton,&exportButton,&savePresetButton,&savePresetAsButton,&loadPresetButton,&advancedButton,&aboutButton,&tooltipsButton})
    {
        configureButton(*b);
        addAndMakeVisible(*b);
    }

    loadSourceButton.onClick=[this]{chooseSource();};
    previewButton.onClick=[this]{startPreview();};
    randomizeButton.onClick=[this]{randomize();};
    mutateButton.onClick=[this]{mutate();};
    undoButton.onClick=[this]{undo();};
    resetButton.onClick=[this]{resetParameters();};
    exportButton.onClick=[this]{exportWav();};
    savePresetButton.onClick=[this]{savePreset();};
    savePresetAsButton.onClick=[this]{savePresetAs();};
    loadPresetButton.onClick=[this]{loadPreset();};
    advancedButton.onClick=[this]{toggleAdvanced();};
    aboutButton.onClick=[this]{showAbout();};
    tooltipsButton.onClick=[this]{toggleTooltips();};

    loadSourceButton.setTooltip("Load or drop one WAV or AIFF source.");
    snareButton.setTooltip("Render the loaded source toward a snare character.");
    clapButton.setTooltip("Render the loaded source toward a clap character.");
    previewButton.setTooltip("Audition the exact active render.");
    randomizeButton.setTooltip("Generate a new bounded variation.");
    mutateButton.setTooltip("Make a smaller variation from the current settings.");
    undoButton.setTooltip("Restore the settings from before the last variation.");
    resetButton.setTooltip("Restore default settings for the selected mode.");
    exportButton.setTooltip("Export the active render as a WAV file.");
    savePresetButton.setTooltip("Save settings to the current user preset.");
    savePresetAsButton.setTooltip("Save settings to a new JSON preset.");
    loadPresetButton.setTooltip("Load a user preset from a JSON file.");
    advancedButton.setTooltip("Show detailed synthesis and WAV export controls.");
    aboutButton.setTooltip("Open the SnairCreator help guide.");
    tooltipsButton.setTooltip("Turn contextual tooltips on or off.");

    presetSelector.setTextWhenNothingSelected("FACTORY PRESETS");
    int presetId=1;
    for(const auto& preset:FactoryPresets::all())
        presetSelector.addItem(preset.name,presetId++);
    presetSelector.onChange=[this]
    {
        const int index=presetSelector.getSelectedId()-1;
        if(index>=0) applyFactoryPreset(index);
    };
    presetSelector.setTooltip("Load a source-independent factory parameter recipe.");
    addAndMakeVisible(presetSelector);

    snareButton.setRadioGroupId(1);
    clapButton.setRadioGroupId(1);
    snareButton.setClickingTogglesState(true);
    clapButton.setClickingTogglesState(true);
    snareButton.onClick=[this]{setMode(false);};
    clapButton.onClick=[this]{setMode(true);};

    modeLabel.setText("OUTPUT CHARACTER",juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId,accent);
    addAndMakeVisible(modeLabel);

    characterLabel.setText("SOURCE CHARACTER",juce::dontSendNotification);
    characterLabel.setColour(juce::Label::textColourId,muted);
    addAndMakeVisible(characterLabel);

    configureSlider(sourceCharacterSlider,0,1,0.01);
    addAndMakeVisible(sourceCharacterSlider);
    sourceCharacterSlider.onDragEnd=[this]{syncParametersFromControls();renderCurrent("Source Character");};

    for(int i=0;i<macroCount;++i)
    {
        auto& l=macroLabels[(size_t)i];
        l.setText(macroNames[(size_t)i],juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId,muted);
        addAndMakeVisible(l);

        auto& s=macroSliders[(size_t)i];
        configureSlider(s,0,1,0.01);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.onDragEnd=[this]{syncParametersFromControls();renderCurrent("Parameter change");};
        addAndMakeVisible(s);
    }

    advancedPanel.onParameterCommit=[this](bool renderRequired)
    {
        syncParametersFromControls();
        playbackTrimGain.store(juce::Decibels::decibelsToGain(parameters.outputTrimDb),std::memory_order_release);
        if(renderRequired)
            renderCurrent("Advanced parameter");
        else
            saveSessionAsync(parameters,std::atomic_load_explicit(&renderedHit,std::memory_order_acquire),
                             source?source->sourceFile:juce::File{});
    };
    advancedPanel.setVisible(false);
    addAndMakeVisible(advancedPanel);

    statusLabel.setText("Load a source to generate a snare or clap.",juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,muted);
    addAndMakeVisible(statusLabel);

    syncControlsFromParameters();
    refreshActionState();
    setAudioChannels(0,2);
    setSize(1000,700);
    restoreSession();
}

MainComponent::~MainComponent()
{
    cancelPendingUpdate();
    ++sourceRequest;
    ++renderRequest;
    sourceWorker.removeAllJobs(true,10000);
    renderWorker.removeAllJobs(true,10000);
    sessionWorker.removeAllJobs(true,10000);
    shutdownAudio();
}

void MainComponent::configureButton(juce::TextButton& b)
{
    b.setColour(juce::TextButton::buttonColourId,panel);
    b.setColour(juce::TextButton::buttonOnColourId,accent.withAlpha(0.2f));
    b.setColour(juce::TextButton::textColourOffId,juce::Colours::white);
}

void MainComponent::configureSlider(juce::Slider& s,double a,double b,double st)
{
    s.setRange(a,b,st);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,20);
    s.setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    auto a=getLocalBounds().reduced(20);
    a.removeFromTop(58);
    for(int h:{190,82,230})
    {
        auto x=a.removeFromTop(h);
        g.setColour(panel);
        g.fillRoundedRectangle(x.toFloat(),10);
        g.setColour(border);
        g.drawRoundedRectangle(x.toFloat(),10,1);
        a.removeFromTop(10);
    }
}

void MainComponent::resized()
{
    auto a=getLocalBounds().reduced(20);
    auto head=a.removeFromTop(48);
    titleLabel.setBounds(head.removeFromLeft(300));
    tooltipsButton.setBounds(head.removeFromRight(96).reduced(2,6));
    aboutButton.setBounds(head.removeFromRight(72).reduced(2,6));
    buildLabel.setBounds(head.removeFromRight(250));
    a.removeFromTop(10);

    auto src=a.removeFromTop(190).reduced(14);
    sectionSourceLabel.setBounds(src.removeFromTop(18));
    auto top=src.removeFromTop(30);
    sourceNameLabel.setBounds(top.removeFromLeft(420));
    loadSourceButton.setBounds(top.removeFromRight(150));
    sourceDetailsLabel.setBounds(src.removeFromTop(20));
    waveform.setBounds(src.removeFromTop(80));
    sourceHintLabel.setBounds(src.removeFromTop(18));
    a.removeFromTop(10);

    auto mode=a.removeFromTop(82).reduced(14);
    modeLabel.setBounds(mode.removeFromTop(18));
    auto mr=mode.removeFromTop(36);
    snareButton.setBounds(mr.removeFromLeft(130));
    mr.removeFromLeft(8);
    clapButton.setBounds(mr.removeFromLeft(130));
    advancedButton.setBounds(mr.removeFromRight(110));
    mr.removeFromRight(8);
    characterLabel.setBounds(mr.removeFromLeft(150));
    sourceCharacterSlider.setBounds(mr.removeFromLeft(std::min(300,mr.getWidth())));
    a.removeFromTop(10);

    auto ctr=a.removeFromTop(230).reduced(14);
    auto editorArea=ctr.removeFromTop(143);
    if(advancedVisible)
    {
        advancedPanel.setBounds(editorArea);
        for(int i=0;i<macroCount;++i)
        {
            macroLabels[(size_t)i].setBounds({});
            macroSliders[(size_t)i].setBounds({});
        }
    }
    else
    {
        advancedPanel.setBounds({});
        auto labels=editorArea.removeFromTop(18);
        auto knobs=editorArea;
        int w=knobs.getWidth()/macroCount;
        for(int i=0;i<macroCount;++i)
        {
            macroLabels[(size_t)i].setBounds(labels.removeFromLeft(w));
            macroSliders[(size_t)i].setBounds(knobs.removeFromLeft(w).reduced(4));
        }
    }

    auto actions=ctr.removeFromTop(36);
    for(auto* b:{&previewButton,&randomizeButton,&mutateButton,&undoButton,&resetButton})
    {
        b->setBounds(actions.removeFromLeft(105));
        actions.removeFromLeft(6);
    }

    a.removeFromTop(10);
    auto bottom=a.removeFromTop(38);
    exportButton.setBounds(bottom.removeFromRight(120));
    bottom.removeFromRight(5);
    savePresetAsButton.setBounds(bottom.removeFromRight(86));
    bottom.removeFromRight(5);
    savePresetButton.setBounds(bottom.removeFromRight(72));
    bottom.removeFromRight(5);
    loadPresetButton.setBounds(bottom.removeFromRight(72));
    bottom.removeFromRight(5);
    presetSelector.setBounds(bottom.removeFromRight(170));
    bottom.removeFromRight(8);
    statusLabel.setBounds(bottom);
}

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& f)
{
    return f.size()==1;
}

void MainComponent::filesDropped(const juce::StringArray& f,int,int)
{
    if(f.size()==1) loadSource(juce::File(f[0]));
}

void MainComponent::chooseSource()
{
    fileChooser=std::make_unique<juce::FileChooser>("Choose source",juce::File{},"*.wav;*.aif;*.aiff");
    juce::Component::SafePointer<MainComponent> safe(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe](const juce::FileChooser& chooser)
        {
            if(safe && chooser.getResult().existsAsFile()) safe->loadSource(chooser.getResult());
        });
}

void MainComponent::loadSource(const juce::File& file)
{
    recoverySuperseded.store(true,std::memory_order_release);
    const auto request=++sourceRequest;
    ++renderRequest;
    renderWorker.removeAllJobs(false,0);
    sourceWorker.removeAllJobs(false,0);
    statusLabel.setText("Decoding and analyzing source…",juce::dontSendNotification);

    juce::Component::SafePointer<MainComponent> safe(this);
    sourceWorker.addJob([safe,file,request]
    {
        juce::String error;
        auto candidate=SourceLoader::decode(file,error);
        SourceAnalysis candidateAnalysis;
        if(candidate && !SourceAnalyzer::analyze(*candidate,candidateAnalysis,error))
            candidate.reset();

        juce::MessageManager::callAsync([safe,file,request,candidate,candidateAnalysis,error]
        {
            if(!safe || request!=safe->sourceRequest.load()) return;

            if(!candidate)
            {
                safe->statusLabel.setText(error.isNotEmpty()?error:"The source could not be loaded.",juce::dontSendNotification);
                safe->refreshActionState();
                return;
            }

            safe->source=candidate;
            safe->analysis=candidateAnalysis;
            safe->sourceNameLabel.setText(file.getFileName(),juce::dontSendNotification);
            safe->sourceDetailsLabel.setText(
                juce::String(candidate->channelCount)+" ch • "
                +juce::String((int)candidate->sampleRate)+" Hz • "
                +juce::String(candidate->durationSeconds(),2)+" s • crest "
                +juce::String(candidateAnalysis.crest,2),
                juce::dontSendNotification);
            safe->sourceHintLabel.setText(
                "Strongest transient "
                +juce::String(candidateAnalysis.strongestTransientSample/candidateAnalysis.sampleRate,3)
                +" s • centroid "+juce::String((int)candidateAnalysis.spectralCentroidHz)+" Hz",
                juce::dontSendNotification);
            safe->waveform.setPeaks(candidate->waveformPeaks);
            safe->waveform.setTransientPosition(static_cast<float>(
                static_cast<double>(candidateAnalysis.strongestTransientSample)
                /std::max<int64_t>(1,candidate->frameCount)));
            safe->refreshActionState();
            safe->renderCurrent("Initial render");
        });
    });
}

void MainComponent::renderCurrent(const juce::String& reason)
{
    if(!source) return;

    syncParametersFromControls();
    playbackTrimGain.store(juce::Decibels::decibelsToGain(parameters.outputTrimDb),std::memory_order_release);

    const auto request=++renderRequest;
    const auto generationId=++generation;
    const auto sourceSnapshot=source;
    const auto analysisSnapshot=analysis;
    const auto parameterSnapshot=parameters;
    const auto targetRate=playbackSampleRate.load(std::memory_order_acquire);

    renderWorker.removeAllJobs(false,0);
    statusLabel.setText("Rendering "+reason+"…",juce::dontSendNotification);
    exportButton.setEnabled(false);

    juce::Component::SafePointer<MainComponent> safe(this);
    renderWorker.addJob([safe,sourceSnapshot,analysisSnapshot,parameterSnapshot,request,generationId,targetRate]
    {
        juce::String error;
        auto hit=SnairEngine::render(*sourceSnapshot,analysisSnapshot,parameterSnapshot,generationId,error,targetRate);
        juce::MessageManager::callAsync([safe,hit,error,request,parameterSnapshot]
        {
            if(!safe || request!=safe->renderRequest.load()) return;

            if(!hit)
            {
                safe->statusLabel.setText("Render failed: "+error,juce::dontSendNotification);
                safe->refreshActionState();
                return;
            }

            safe->std::atomic_store_explicit(&renderedHit,RenderedHitPtr(hit),std::memory_order_release);
            safe->playbackTrimGain.store(
                juce::Decibels::decibelsToGain(parameterSnapshot.outputTrimDb),
                std::memory_order_release);
            safe->statusLabel.setText(
                juce::String(parameterSnapshot.mode==SnairMode::clap?"Clap":"Snare")
                +" ready • peak "+juce::String(hit->peak,3)
                +(hit->usedFallback?" • reinforced source":""),
                juce::dontSendNotification);
            safe->refreshActionState();
            safe->saveSessionAsync(parameterSnapshot,hit,
                                   safe->source?safe->source->sourceFile:juce::File{});
        });
    });
}

void MainComponent::refreshActionState()
{
    const bool hasSource=static_cast<bool>(source);
    const bool hasHit=static_cast<bool>(std::atomic_load_explicit(&renderedHit,std::memory_order_acquire));
    previewButton.setEnabled(hasHit);
    exportButton.setEnabled(hasHit);
    randomizeButton.setEnabled(hasSource);
    mutateButton.setEnabled(hasSource);
    resetButton.setEnabled(hasSource);
    savePresetButton.setEnabled(hasSource || hasHit);
    undoButton.setEnabled(hasUndo && hasSource);
    snareButton.setEnabled(hasSource);
    clapButton.setEnabled(hasSource);
    sourceCharacterSlider.setEnabled(hasSource);
    for(auto& slider:macroSliders) slider.setEnabled(hasSource);
}

void MainComponent::syncParametersFromControls()
{
    parameters.sourceCharacter=(float)sourceCharacterSlider.getValue();
    parameters.punch=(float)macroSliders[0].getValue();
    parameters.snap=(float)macroSliders[1].getValue();
    parameters.body=(float)macroSliders[2].getValue();
    parameters.texture=(float)macroSliders[3].getValue();
    parameters.dirt=(float)macroSliders[4].getValue();
    parameters.size=(float)macroSliders[5].getValue();
    advancedPanel.applyTo(parameters);
    parameters.sanitize();
}

void MainComponent::syncControlsFromParameters()
{
    sourceCharacterSlider.setValue(parameters.sourceCharacter,juce::dontSendNotification);
    macroSliders[0].setValue(parameters.punch,juce::dontSendNotification);
    macroSliders[1].setValue(parameters.snap,juce::dontSendNotification);
    macroSliders[2].setValue(parameters.body,juce::dontSendNotification);
    macroSliders[3].setValue(parameters.texture,juce::dontSendNotification);
    macroSliders[4].setValue(parameters.dirt,juce::dontSendNotification);
    macroSliders[5].setValue(parameters.size,juce::dontSendNotification);
    snareButton.setToggleState(parameters.mode==SnairMode::snare,juce::dontSendNotification);
    clapButton.setToggleState(parameters.mode==SnairMode::clap,juce::dontSendNotification);
    advancedPanel.setParameters(parameters);
    playbackTrimGain.store(juce::Decibels::decibelsToGain(parameters.outputTrimDb),std::memory_order_release);
}

void MainComponent::setMode(bool clap)
{
    parameters.mode=clap?SnairMode::clap:SnairMode::snare;
    syncControlsFromParameters();
    renderCurrent("Mode change");
}

void MainComponent::startPreview()
{
    if(std::atomic_load_explicit(&renderedHit,std::memory_order_acquire))
    {
        previewPosition.store(0,std::memory_order_release);
        previewActive.store(true,std::memory_order_release);
    }
}

void MainComponent::randomize()
{
    undoParameters=parameters;
    hasUndo=true;
    parameters=SnairEngine::randomized(parameters,++variationCounter);
    syncControlsFromParameters();
    refreshActionState();
    renderCurrent("Randomize");
}

void MainComponent::mutate()
{
    undoParameters=parameters;
    hasUndo=true;
    parameters=SnairEngine::mutated(parameters,++variationCounter);
    syncControlsFromParameters();
    refreshActionState();
    renderCurrent("Mutate");
}

void MainComponent::undo()
{
    if(!hasUndo) return;
    parameters=undoParameters;
    hasUndo=false;
    syncControlsFromParameters();
    refreshActionState();
    renderCurrent("Undo");
}

void MainComponent::resetParameters()
{
    const auto mode=parameters.mode;
    parameters={};
    parameters.mode=mode;
    hasUndo=false;
    syncControlsFromParameters();
    refreshActionState();
    renderCurrent("Reset");
}

void MainComponent::exportWav()
{
    auto hit=std::atomic_load_explicit(&renderedHit,std::memory_order_acquire);
    if(!hit) return;

    syncParametersFromControls();
    const auto p=parameters;
    const auto options=advancedPanel.exportOptions(p.outputTrimDb);
    const auto sourceStem=legalStem(source?source->sourceFile.getFileNameWithoutExtension():"Recovered");
    const auto seed=juce::String(p.seed).paddedLeft('0',4);
    const auto defaultName="SnairCreator_"
        +juce::String(p.mode==SnairMode::clap?"Clap":"Snare")
        +"_"+sourceStem+"_"+seed+".wav";

    fileChooser=std::make_unique<juce::FileChooser>(
        "Export WAV",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(defaultName),
        "*.wav");

    juce::Component::SafePointer<MainComponent> safe(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting,
        [safe,hit,p,options](const juce::FileChooser& chooser)
        {
            if(!safe || chooser.getResult()==juce::File{}) return;
            juce::String error;
            const auto target=chooser.getResult().withFileExtension(".wav");
            if(WavExporter::write(target,*hit,options,error))
            {
                const double rate=options.sampleRate>0.0?options.sampleRate:hit->sampleRate;
                const double duration=hit->samples.getNumSamples()/hit->sampleRate;
                safe->statusLabel.setText(
                    "Exported "+target.getFileName()+" • "
                    +juce::String(duration,3)+" s • "
                    +juce::String((int)rate)+" Hz • "
                    +juce::String(options.bitDepth)+"-bit",
                    juce::dontSendNotification);
            }
            else
                safe->statusLabel.setText(error,juce::dontSendNotification);
        });
}

void MainComponent::savePreset()
{
    syncParametersFromControls();
    if(currentPresetFile==juce::File{})
    {
        savePresetAs();
        return;
    }

    juce::String error;
    if(PresetManager::save(currentPresetFile,parameters,error))
        statusLabel.setText("Saved preset "+currentPresetFile.getFileName(),juce::dontSendNotification);
    else
        statusLabel.setText(error,juce::dontSendNotification);
}

void MainComponent::savePresetAs()
{
    syncParametersFromControls();
    const auto suggested=currentPresetFile==juce::File{}
        ? juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("SnairCreator.preset.json")
        : currentPresetFile;

    fileChooser=std::make_unique<juce::FileChooser>("Save preset as",suggested,"*.json");
    const auto p=parameters;
    juce::Component::SafePointer<MainComponent> safe(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting,
        [safe,p](const juce::FileChooser& chooser)
        {
            if(!safe || chooser.getResult()==juce::File{}) return;
            const auto target=chooser.getResult().withFileExtension(".json");
            juce::String error;
            if(PresetManager::save(target,p,error))
            {
                safe->currentPresetFile=target;
                safe->statusLabel.setText("Saved preset "+target.getFileName(),juce::dontSendNotification);
            }
            else safe->statusLabel.setText(error,juce::dontSendNotification);
        });
}

void MainComponent::loadPreset()
{
    fileChooser=std::make_unique<juce::FileChooser>("Load preset",juce::File{},"*.json");
    juce::Component::SafePointer<MainComponent> safe(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe](const juce::FileChooser& chooser)
        {
            if(!safe || !chooser.getResult().existsAsFile()) return;
            juce::String error;
            auto p=safe->parameters;
            if(PresetManager::load(chooser.getResult(),p,error))
            {
                safe->parameters=p;
                safe->currentPresetFile=chooser.getResult();
                safe->presetSelector.setSelectedId(0,juce::dontSendNotification);
                safe->syncControlsFromParameters();
                if(safe->source)
                    safe->renderCurrent("Preset load");
                else
                    safe->statusLabel.setText("Preset loaded. Load a source to render it.",juce::dontSendNotification);
            }
            else safe->statusLabel.setText(error,juce::dontSendNotification);
        });
}

void MainComponent::prepareToPlay(int,double sampleRate)
{
    const auto previous=playbackSampleRate.exchange(sampleRate,std::memory_order_acq_rel);
    if(std::abs(previous-sampleRate)>0.5) triggerAsyncUpdate();
}

void MainComponent::handleAsyncUpdate()
{
    if(source) renderCurrent("Audio device rate change");
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& out)
{
    out.clearActiveBufferRegion();
    auto hit=std::atomic_load_explicit(&renderedHit,std::memory_order_acquire);
    if(!hit || !previewActive.load(std::memory_order_acquire)) return;

    const int pos=previewPosition.load(std::memory_order_acquire);
    const int remain=hit->samples.getNumSamples()-pos;
    if(remain<=0)
    {
        previewActive.store(false,std::memory_order_release);
        return;
    }

    const int n=std::min(out.numSamples,remain);
    const float trim=playbackTrimGain.load(std::memory_order_acquire);
    for(int ch=0;ch<out.buffer->getNumChannels();++ch)
    {
        const int srcCh=std::min(ch,hit->samples.getNumChannels()-1);
        out.buffer->copyFrom(ch,out.startSample,hit->samples,srcCh,pos,n);
        out.buffer->applyGain(ch,out.startSample,n,trim);
    }

    previewPosition.store(pos+n,std::memory_order_release);
    if(pos+n>=hit->samples.getNumSamples())
        previewActive.store(false,std::memory_order_release);
}

void MainComponent::releaseResources(){}


void MainComponent::showAbout()
{
    const juce::String help =
        "SNAIRCREATOR HELP\\n\\n"
        "GETTING STARTED\\nLoad one WAV or AIFF source with Load Source or drag it into the window. "
        "Mono and stereo files up to 10 minutes are supported. A rejected file leaves the previous source and render available.\\n\\n"
        "SOURCE AND ANALYSIS\\nThe waveform and transient marker summarize the loaded audio. Source Character blends recognizable source material with generated reinforcement.\\n\\n"
        "SOUND CONTROLS\\nSnare and Clap choose the render character. Punch, Snap, Body, Texture, Dirt, and Size shape the sound. "
        "Advanced controls set body frequency, attack, noise, tail, pitch, tone, drive, width, clap pattern, and output trim.\\n\\n"
        "ACTIONS\\nPreview plays the active render. Randomize creates a new bounded variation; Mutate makes a smaller change; Undo restores the prior settings; Reset restores defaults. "
        "Factory and user presets save reusable parameter recipes.\\n\\n"
        "EXPORT\\nExport WAV writes the active render. Advanced export options select PCM 16-bit, PCM 24-bit, or float 32-bit, sample rate, channel count, normalization, and output trim.\\n\\n"
        "RECOVERY\\nThe latest render and settings are recovered at startup. If the source file has moved, load it again to continue editing; the recovered WAV remains playable and exportable.\\n\\n"
        "TOOLTIPS\\nUse TIPS ON / TIPS OFF in the title bar to enable or disable contextual hover help.";
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                           "SnairCreator Help",
                                           help,
                                           "Close");
}

void MainComponent::toggleTooltips()
{
    tooltipsEnabled=!tooltipsEnabled;
    if(tooltipsEnabled)
        tooltipWindow=std::make_unique<juce::TooltipWindow>(nullptr,700);
    else
        tooltipWindow.reset();
    tooltipsButton.setButtonText(tooltipsEnabled?"TIPS ON":"TIPS OFF");
}

void MainComponent::toggleAdvanced()
{
    advancedVisible=!advancedVisible;
    advancedPanel.setVisible(advancedVisible);
    for(int i=0;i<macroCount;++i)
    {
        macroLabels[(size_t)i].setVisible(!advancedVisible);
        macroSliders[(size_t)i].setVisible(!advancedVisible);
    }
    advancedButton.setToggleState(advancedVisible,juce::dontSendNotification);
    resized();
}

void MainComponent::saveSessionAsync(const SnairParameters& p,
                                     const RenderedHitPtr& hit,
                                     const juce::File& sourceFile)
{
    if(!hit) return;
    sessionWorker.removeAllJobs(false,0);
    const auto directory=SessionStore::defaultDirectory();
    sessionWorker.addJob([directory,p,hit,sourceFile]
    {
        juce::String ignored;
        SessionStore::save(directory,p,hit,sourceFile,ignored);
    });
}

void MainComponent::restoreSession()
{
    const auto directory=SessionStore::defaultDirectory();
    juce::Component::SafePointer<MainComponent> safe(this);
    sessionWorker.addJob([safe,directory]
    {
        RestoredSession restored;
        juce::String error;
        const bool found=SessionStore::load(directory,restored,error);
        juce::MessageManager::callAsync([safe,found,restored,error]
        {
            if(!safe || safe->recoverySuperseded.load(std::memory_order_acquire)) return;
            if(!found)
            {
                if(error.isNotEmpty())
                    safe->statusLabel.setText("Recovery ignored: "+error,juce::dontSendNotification);
                return;
            }

            safe->parameters=restored.parameters;
            safe->syncControlsFromParameters();
            if(restored.hit)
            {
                safe->std::atomic_store_explicit(&renderedHit,restored.hit,std::memory_order_release);
                safe->sourceNameLabel.setText("Recovered previous render",juce::dontSendNotification);
                safe->sourceDetailsLabel.setText(
                    juce::String(restored.hit->samples.getNumChannels())+" ch • "
                    +juce::String((int)restored.hit->sampleRate)+" Hz • recovered session",
                    juce::dontSendNotification);
            }

            safe->refreshActionState();
            if(restored.sourceFile.existsAsFile())
            {
                safe->statusLabel.setText("Recovered previous render; relinking source…",juce::dontSendNotification);
                safe->loadSource(restored.sourceFile);
            }
            else if(restored.hit)
            {
                safe->sourceHintLabel.setText("Original source is unavailable; recovered hit remains playable and exportable.",
                                              juce::dontSendNotification);
                safe->statusLabel.setText("Recovered previous sound. Relink by loading its original source.",
                                          juce::dontSendNotification);
            }
        });
    });
}


void MainComponent::applyFactoryPreset(int index)
{
    const auto& presets=FactoryPresets::all();
    if(index<0 || index>=static_cast<int>(presets.size())) return;
    undoParameters=parameters;
    hasUndo=true;
    parameters=presets[(size_t)index].parameters;
    currentPresetFile={};
    syncControlsFromParameters();
    refreshActionState();
    if(source)
        renderCurrent("Factory preset");
    else
        statusLabel.setText("Factory preset selected. Load a source to render it.",juce::dontSendNotification);
}
