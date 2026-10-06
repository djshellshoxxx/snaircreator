#include "MainComponent.h"
#include "SourceAnalyzer.h"
#include "SourceLoader.h"
#include "SnairEngine.h"
#include "WavExporter.h"

namespace {
const juce::Colour bg{15,18,23}, panel{24,29,37}, border{48,58,70}, accent{47,202,224}, muted{145,157,171};
}

MainComponent::MainComponent()
{
    titleLabel.setText("SnairCreator",juce::dontSendNotification); titleLabel.setFont(juce::Font(27.0f,juce::Font::bold)); titleLabel.setColour(juce::Label::textColourId,juce::Colours::white); addAndMakeVisible(titleLabel);
    buildLabel.setText("STANDALONE  /  COMPLETE ENGINE",juce::dontSendNotification); buildLabel.setFont(juce::Font(11.0f,juce::Font::bold)); buildLabel.setColour(juce::Label::textColourId,accent); addAndMakeVisible(buildLabel);
    sectionSourceLabel.setText("SOURCE AUDIO",juce::dontSendNotification); sectionSourceLabel.setColour(juce::Label::textColourId,accent); addAndMakeVisible(sectionSourceLabel);
    sourceNameLabel.setText("No source loaded",juce::dontSendNotification); sourceNameLabel.setColour(juce::Label::textColourId,juce::Colours::white); addAndMakeVisible(sourceNameLabel);
    sourceDetailsLabel.setText("WAV or AIFF • one file at a time",juce::dontSendNotification); sourceDetailsLabel.setColour(juce::Label::textColourId,muted); addAndMakeVisible(sourceDetailsLabel);
    sourceHintLabel.setText("Drop a WAV or AIFF here",juce::dontSendNotification); sourceHintLabel.setColour(juce::Label::textColourId,muted); addAndMakeVisible(sourceHintLabel);
    addAndMakeVisible(waveform);

    for(auto* b:{&loadSourceButton,&snareButton,&clapButton,&previewButton,&randomizeButton,&mutateButton,&undoButton,&resetButton,&exportButton,&savePresetButton,&loadPresetButton}){configureButton(*b);addAndMakeVisible(*b);}
    loadSourceButton.onClick=[this]{chooseSource();}; previewButton.onClick=[this]{startPreview();}; randomizeButton.onClick=[this]{randomize();}; mutateButton.onClick=[this]{mutate();}; undoButton.onClick=[this]{undo();}; resetButton.onClick=[this]{resetParameters();}; exportButton.onClick=[this]{exportWav();}; savePresetButton.onClick=[this]{savePreset();}; loadPresetButton.onClick=[this]{loadPreset();};

    snareButton.setRadioGroupId(1); clapButton.setRadioGroupId(1); snareButton.setClickingTogglesState(true); clapButton.setClickingTogglesState(true);
    snareButton.onClick=[this]{setMode(false);}; clapButton.onClick=[this]{setMode(true);};

    modeLabel.setText("OUTPUT CHARACTER",juce::dontSendNotification); modeLabel.setColour(juce::Label::textColourId,accent); addAndMakeVisible(modeLabel);
    characterLabel.setText("SOURCE CHARACTER",juce::dontSendNotification); characterLabel.setColour(juce::Label::textColourId,muted); addAndMakeVisible(characterLabel);
    configureSlider(sourceCharacterSlider,0,1,0.01); addAndMakeVisible(sourceCharacterSlider);
    sourceCharacterSlider.onDragEnd=[this]{syncParametersFromControls();renderCurrent("Source Character");};
    for(int i=0;i<macroCount;++i){ auto& l=macroLabels[(size_t)i]; l.setText(macroNames[(size_t)i],juce::dontSendNotification);l.setJustificationType(juce::Justification::centred);l.setColour(juce::Label::textColourId,muted);addAndMakeVisible(l); auto& s=macroSliders[(size_t)i];configureSlider(s,0,1,0.01);s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.onDragEnd=[this]{syncParametersFromControls();renderCurrent("Parameter change");};addAndMakeVisible(s);}

    statusLabel.setText("Load a source to generate a snare or clap.",juce::dontSendNotification);statusLabel.setColour(juce::Label::textColourId,muted);addAndMakeVisible(statusLabel);
    syncControlsFromParameters();
    previewButton.setEnabled(false);randomizeButton.setEnabled(false);mutateButton.setEnabled(false);undoButton.setEnabled(false);resetButton.setEnabled(false);exportButton.setEnabled(false);savePresetButton.setEnabled(false);
    setAudioChannels(0,2);
    setSize(1000,700);
}

MainComponent::~MainComponent(){shutdownAudio();}
void MainComponent::configureButton(juce::TextButton& b){b.setColour(juce::TextButton::buttonColourId,panel);b.setColour(juce::TextButton::buttonOnColourId,accent.withAlpha(0.2f));b.setColour(juce::TextButton::textColourOffId,juce::Colours::white);}
void MainComponent::configureSlider(juce::Slider& s,double a,double b,double st){s.setRange(a,b,st);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,20);s.setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);}

void MainComponent::paint(juce::Graphics& g){g.fillAll(bg);auto a=getLocalBounds().reduced(20);a.removeFromTop(58);for(int h:{190,82,230}){auto x=a.removeFromTop(h);g.setColour(panel);g.fillRoundedRectangle(x.toFloat(),10);g.setColour(border);g.drawRoundedRectangle(x.toFloat(),10,1);a.removeFromTop(10);}}
void MainComponent::resized()
{
    auto a=getLocalBounds().reduced(20);auto head=a.removeFromTop(48);titleLabel.setBounds(head.removeFromLeft(300));buildLabel.setBounds(head.removeFromRight(250));a.removeFromTop(10);
    auto src=a.removeFromTop(190).reduced(14);sectionSourceLabel.setBounds(src.removeFromTop(18));auto top=src.removeFromTop(30);sourceNameLabel.setBounds(top.removeFromLeft(420));loadSourceButton.setBounds(top.removeFromRight(150));sourceDetailsLabel.setBounds(src.removeFromTop(20));waveform.setBounds(src.removeFromTop(80));sourceHintLabel.setBounds(src.removeFromTop(18));a.removeFromTop(10);
    auto mode=a.removeFromTop(82).reduced(14);modeLabel.setBounds(mode.removeFromTop(18));auto mr=mode.removeFromTop(36);snareButton.setBounds(mr.removeFromLeft(130));mr.removeFromLeft(8);clapButton.setBounds(mr.removeFromLeft(130));characterLabel.setBounds(mr.removeFromLeft(150));sourceCharacterSlider.setBounds(mr.removeFromLeft(300));a.removeFromTop(10);
    auto ctr=a.removeFromTop(230).reduced(14);auto labels=ctr.removeFromTop(18);auto knobs=ctr.removeFromTop(125);int w=knobs.getWidth()/macroCount;for(int i=0;i<macroCount;++i){auto lc=labels.removeFromLeft(w);macroLabels[(size_t)i].setBounds(lc);auto kc=knobs.removeFromLeft(w);macroSliders[(size_t)i].setBounds(kc.reduced(4));}
    auto actions=ctr.removeFromTop(36);for(auto* b:{&previewButton,&randomizeButton,&mutateButton,&undoButton,&resetButton}){b->setBounds(actions.removeFromLeft(105));actions.removeFromLeft(6);}a.removeFromTop(10);
    auto bottom=a.removeFromTop(38);exportButton.setBounds(bottom.removeFromRight(130));bottom.removeFromRight(6);savePresetButton.setBounds(bottom.removeFromRight(120));bottom.removeFromRight(6);loadPresetButton.setBounds(bottom.removeFromRight(120));statusLabel.setBounds(bottom);
}

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& f){return f.size()==1;}
void MainComponent::filesDropped(const juce::StringArray& f,int,int){if(f.size()==1)loadSource(juce::File(f[0]));}
void MainComponent::chooseSource(){fileChooser=std::make_unique<juce::FileChooser>("Choose source",juce::File{},"*.wav;*.aif;*.aiff");juce::Component::SafePointer<MainComponent> s(this);fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[s](const juce::FileChooser& c){if(s&&c.getResult().existsAsFile())s->loadSource(c.getResult());});}

void MainComponent::loadSource(const juce::File& f)
{
    statusLabel.setText("Decoding and analyzing source…",juce::dontSendNotification);
    juce::String err;auto candidate=SourceLoader::decode(f,err);if(!candidate){statusLabel.setText(err,juce::dontSendNotification);return;}
    SourceAnalysis a;if(!SourceAnalyzer::analyze(*candidate,a,err)){statusLabel.setText(err,juce::dontSendNotification);return;}
    source=candidate;analysis=a;sourceNameLabel.setText(f.getFileName(),juce::dontSendNotification);
    sourceDetailsLabel.setText(juce::String(candidate->channelCount)+" ch • "+juce::String((int)candidate->sampleRate)+" Hz • "+juce::String(candidate->durationSeconds(),2)+" s • crest "+juce::String(a.crest,2),juce::dontSendNotification);
    sourceHintLabel.setText("Strongest transient "+juce::String(a.strongestTransientSample/a.sampleRate,3)+" s • centroid "+juce::String((int)a.spectralCentroidHz)+" Hz",juce::dontSendNotification);
    waveform.setPeaks(candidate->waveformPeaks);renderCurrent("Initial render");
    randomizeButton.setEnabled(true);mutateButton.setEnabled(true);resetButton.setEnabled(true);savePresetButton.setEnabled(true);
}

void MainComponent::renderCurrent(const juce::String& reason)
{
    if(!source)return;syncParametersFromControls();statusLabel.setText("Rendering "+reason+"…",juce::dontSendNotification);juce::String err;auto h=SnairEngine::render(*source,analysis,parameters,++generation,err);
    if(!h){statusLabel.setText("Render failed: "+err,juce::dontSendNotification);return;}renderedHit=h;previewButton.setEnabled(true);exportButton.setEnabled(true);statusLabel.setText((parameters.mode==SnairMode::clap?"Clap":"Snare")+juce::String(" ready • peak ")+juce::String(h->peak,3),juce::dontSendNotification);
}

void MainComponent::syncParametersFromControls(){parameters.sourceCharacter=(float)sourceCharacterSlider.getValue();parameters.punch=(float)macroSliders[0].getValue();parameters.snap=(float)macroSliders[1].getValue();parameters.body=(float)macroSliders[2].getValue();parameters.texture=(float)macroSliders[3].getValue();parameters.dirt=(float)macroSliders[4].getValue();parameters.size=(float)macroSliders[5].getValue();parameters.sanitize();}
void MainComponent::syncControlsFromParameters(){sourceCharacterSlider.setValue(parameters.sourceCharacter,juce::dontSendNotification);macroSliders[0].setValue(parameters.punch,juce::dontSendNotification);macroSliders[1].setValue(parameters.snap,juce::dontSendNotification);macroSliders[2].setValue(parameters.body,juce::dontSendNotification);macroSliders[3].setValue(parameters.texture,juce::dontSendNotification);macroSliders[4].setValue(parameters.dirt,juce::dontSendNotification);macroSliders[5].setValue(parameters.size,juce::dontSendNotification);snareButton.setToggleState(parameters.mode==SnairMode::snare,juce::dontSendNotification);clapButton.setToggleState(parameters.mode==SnairMode::clap,juce::dontSendNotification);}
void MainComponent::setMode(bool clap){parameters.mode=clap?SnairMode::clap:SnairMode::snare;syncControlsFromParameters();renderCurrent("Mode change");}
void MainComponent::startPreview(){if(renderedHit){previewPosition=0;previewActive=true;}}
void MainComponent::randomize(){undoParameters=parameters;hasUndo=true;parameters=SnairEngine::randomized(parameters,++variationCounter);syncControlsFromParameters();undoButton.setEnabled(true);renderCurrent("Randomize");}
void MainComponent::mutate(){undoParameters=parameters;hasUndo=true;parameters=SnairEngine::mutated(parameters,++variationCounter);syncControlsFromParameters();undoButton.setEnabled(true);renderCurrent("Mutate");}
void MainComponent::undo(){if(!hasUndo)return;parameters=undoParameters;hasUndo=false;undoButton.setEnabled(false);syncControlsFromParameters();renderCurrent("Undo");}
void MainComponent::resetParameters(){auto m=parameters.mode;parameters={};parameters.mode=m;syncControlsFromParameters();renderCurrent("Reset");}

void MainComponent::exportWav()
{
    if(!renderedHit)return;fileChooser=std::make_unique<juce::FileChooser>("Export WAV",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("SnairCreator_"+juce::String(parameters.mode==SnairMode::clap?"Clap":"Snare")+"_"+juce::String(parameters.seed)+".wav"),"*.wav");
    auto hit=renderedHit;auto p=parameters;juce::Component::SafePointer<MainComponent> s(this);fileChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting,[s,hit,p](const juce::FileChooser& c){if(!s||c.getResult()==juce::File{})return;WavExportOptions o;o.outputTrimDb=p.outputTrimDb;juce::String e;if(WavExporter::write(c.getResult(),*hit,o,e))s->statusLabel.setText("Exported "+c.getResult().getFileName(),juce::dontSendNotification);else s->statusLabel.setText(e,juce::dontSendNotification);});
}
void MainComponent::savePreset(){fileChooser=std::make_unique<juce::FileChooser>("Save preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("SnairCreator.preset.json"),"*.json");auto p=parameters;juce::Component::SafePointer<MainComponent>s(this);fileChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting,[s,p](const juce::FileChooser& c){if(!s||c.getResult()==juce::File{})return;juce::String e;s->statusLabel.setText(PresetManager::save(c.getResult(),p,e)?"Preset saved":e,juce::dontSendNotification);});}
void MainComponent::loadPreset(){fileChooser=std::make_unique<juce::FileChooser>("Load preset",juce::File{},"*.json");juce::Component::SafePointer<MainComponent>s(this);fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[s](const juce::FileChooser& c){if(!s||!c.getResult().existsAsFile())return;juce::String e;auto p=s->parameters;if(PresetManager::load(c.getResult(),p,e)){s->parameters=p;s->syncControlsFromParameters();s->renderCurrent("Preset load");}else s->statusLabel.setText(e,juce::dontSendNotification);});}

void MainComponent::prepareToPlay(int,double){}
void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& out)
{
    out.clearActiveBufferRegion();auto h=renderedHit;if(!h||!previewActive)return;int pos=previewPosition.load();const int remain=h->samples.getNumSamples()-pos;if(remain<=0){previewActive=false;return;}const int n=std::min(out.numSamples,remain);const float trim=juce::Decibels::decibelsToGain(parameters.outputTrimDb);
    for(int ch=0;ch<out.buffer->getNumChannels();++ch){int srcCh=std::min(ch,h->samples.getNumChannels()-1);out.buffer->copyFrom(ch,out.startSample,h->samples,srcCh,pos,n);out.buffer->applyGain(ch,out.startSample,n,trim);}previewPosition=pos+n;if(pos+n>=h->samples.getNumSamples())previewActive=false;
}
void MainComponent::releaseResources(){}
