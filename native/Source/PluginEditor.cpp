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

    const char* macroNames[] = {"Punch", "Snap", "Dirt", "Size"};
    const float macroDefaults[] = {.68f, .66f, .22f, .22f};
    for (int i=0;i<4;++i) {
        macroLabels[static_cast<size_t>(i)].setText(macroNames[i], juce::dontSendNotification);
        macroLabels[static_cast<size_t>(i)].setJustificationType(juce::Justification::centred);
        macroLabels[static_cast<size_t>(i)].setColour(juce::Label::textColourId, juce::Colour(cyan)); addAndMakeVisible(macroLabels[static_cast<size_t>(i)]);
        auto& s=macros[static_cast<size_t>(i)]; s.setRange(0.0,1.0,.001); s.setValue(macroDefaults[i], juce::dontSendNotification);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,62,18);
        s.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(cyan)); s.setColour(juce::Slider::textBoxTextColourId,juce::Colour(text)); s.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(line));
        s.onValueChange=[this,i]{ applyMacro(i, static_cast<float>(macros[static_cast<size_t>(i)].getValue())); }; addAndMakeVisible(s);
    }

    const std::pair<const char*, const char*> defs[] = {
        {"Character","character"},{"Body","body"},{"Body Hz","body_freq_hz"},{"Crack","crack"},{"Noise","noise"},
        {"Tail ms","tail_ms"},{"Clap count","clap_count"},{"Clap spread","clap_spread_ms"},{"Width","width"},
        {"Drive dB","drive_db"},{"Tone","tone"},{"Pitch st","pitch_st"},{"Trim dB","trim_db"},{"Output ms","output_ms"}
    };
    for (auto& d : defs) addControl(d.first, d.second);

    for (auto* b : {&load,&preview,&randomize,&mutate,&undo,&reset,&exportWav,&savePreset,&loadPreset}) {
        addAndMakeVisible(*b); b->setColour(juce::TextButton::buttonColourId, juce::Colour(panel)); b->setColour(juce::TextButton::textColourOffId, juce::Colour(text));
    }
    undo.setEnabled(false);
    load.onClick=[this]{chooseSource();}; preview.onClick=[this]{processor.triggerPreview();};
    randomize.onClick=[this]{randomizeParameters(false);}; mutate.onClick=[this]{randomizeParameters(true);}; undo.onClick=[this]{restoreUndo();};
    reset.onClick=[this]{resetParameters();}; exportWav.onClick=[this]{chooseExport();}; savePreset.onClick=[this]{choosePresetSave();}; loadPreset.onClick=[this]{choosePresetLoad();};
    startTimerHz(8);
}

void SnairCreatorAudioProcessorEditor::addControl(const juce::String& name, const juce::String& id) {
    auto c=std::make_unique<Control>(); c->label.setText(name,juce::dontSendNotification); c->label.setColour(juce::Label::textColourId,juce::Colour(muted));
    c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,20);
    c->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(cyan)); c->slider.setColour(juce::Slider::textBoxTextColourId,juce::Colour(text)); c->slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(line));
    addAndMakeVisible(c->label); addAndMakeVisible(c->slider); c->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,id,c->slider); controls.push_back(std::move(c));
}

void SnairCreatorAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(bg)); auto r=getLocalBounds().reduced(18).toFloat();
    g.setColour(juce::Colour(panel)); g.fillRoundedRectangle(r.withTrimmedTop(105),16.0f); g.setColour(juce::Colour(line)); g.drawRoundedRectangle(r.withTrimmedTop(105),16.0f,1.0f);
    if(!waveformBounds.isEmpty()) {
        auto wr=waveformBounds.toFloat(); g.setColour(juce::Colour(0xff080b10)); g.fillRoundedRectangle(wr,8.0f);
        auto wave=processor.getSourcePreview(std::max(32,waveformBounds.getWidth()/3));
        if(!wave.empty()){juce::Path path; const float mid=wr.getCentreY(),half=wr.getHeight()*.42f;for(size_t i=0;i<wave.size();++i){const float x=wr.getX()+wr.getWidth()*static_cast<float>(i)/std::max<size_t>(1,wave.size()-1),y=mid-wave[i]*half;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(juce::Colour(cyan));g.strokePath(path,juce::PathStrokeType(1.3f));}
    }
}

void SnairCreatorAudioProcessorEditor::resized() {
    auto r=getLocalBounds().reduced(22); title.setBounds(r.removeFromTop(44)); subtitle.setBounds(r.removeFromTop(26));
    auto sourceBlock=r.removeFromTop(82); auto sourceTop=sourceBlock.removeFromTop(34); load.setBounds(sourceTop.removeFromLeft(120)); sourceTop.removeFromLeft(10); sourceInfo.setBounds(sourceTop); waveformBounds=sourceBlock.reduced(0,4);
    auto modeRow=r.removeFromTop(38).reduced(8,3); mode.setBounds(modeRow.removeFromLeft(170)); normalize.setBounds(modeRow.removeFromLeft(120));
    auto macroArea=r.removeFromTop(100).reduced(8,2); const int mw=macroArea.getWidth()/4;
    for(int i=0;i<4;++i){auto cell=macroArea.removeFromLeft(i==3?macroArea.getWidth():mw).reduced(4);macroLabels[static_cast<size_t>(i)].setBounds(cell.removeFromTop(20));macros[static_cast<size_t>(i)].setBounds(cell);}
    const int cols=getWidth()<900?4:7, rows=static_cast<int>((controls.size()+cols-1)/cols); const int reserved=86;
    auto controlsArea=r.removeFromTop(std::max(110,std::min(std::max(110,r.getHeight()-reserved),rows*120))); const int w=controlsArea.getWidth()/cols,h=std::max(90,controlsArea.getHeight()/std::max(1,rows));
    for(size_t i=0;i<controls.size();++i){const int col=static_cast<int>(i)%cols,row=static_cast<int>(i)/cols;auto cell=juce::Rectangle<int>(controlsArea.getX()+col*w,controlsArea.getY()+row*h,w,h).reduced(4);controls[i]->label.setBounds(cell.removeFromTop(20));controls[i]->slider.setBounds(cell);}
    auto row1=r.removeFromTop(38); const int bw=std::max(88,(row1.getWidth()-28)/5); for(auto* b:{&preview,&randomize,&mutate,&undo,&reset}){b->setBounds(row1.removeFromLeft(bw));row1.removeFromLeft(7);}
    auto row2=r.removeFromTop(38); const int bw2=std::max(100,(row2.getWidth()-21)/4); for(auto* b:{&exportWav,&savePreset,&loadPreset}){b->setBounds(row2.removeFromLeft(bw2));row2.removeFromLeft(7);}
}

void SnairCreatorAudioProcessorEditor::filesDropped(const juce::StringArray& files,int,int){if(files.isEmpty())return;if(!processor.loadSourceFile(juce::File(files[0])))sourceInfo.setText("Could not decode dropped file",juce::dontSendNotification);repaint();}

void SnairCreatorAudioProcessorEditor::chooseSource(){chooser=std::make_unique<juce::FileChooser>("Load source audio",juce::File{},"*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.m4a");chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){auto f=fc.getResult();if(f.existsAsFile())processor.loadSourceFile(f);repaint();});}
void SnairCreatorAudioProcessorEditor::chooseExport(){chooser=std::make_unique<juce::FileChooser>("Export generated hit",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("snaircreator.wav"),"*.wav");chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser& fc){auto f=fc.getResult();if(f.getFileExtension().isEmpty())f=f.withFileExtension("wav");if(f!=juce::File{})processor.exportRendered(f,24);});}

void SnairCreatorAudioProcessorEditor::choosePresetSave(){chooser=std::make_unique<juce::FileChooser>("Save SnairCreator preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("SnairCreator.snairpreset"),"*.snairpreset");chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser& fc){auto f=fc.getResult();if(f.getFileExtension().isEmpty())f=f.withFileExtension("snairpreset");if(f==juce::File{})return;juce::MemoryBlock data;processor.getStateInformation(data);f.replaceWithData(data.getData(),data.getSize());});}
void SnairCreatorAudioProcessorEditor::choosePresetLoad(){chooser=std::make_unique<juce::FileChooser>("Load SnairCreator preset",juce::File{},"*.snairpreset");chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){auto f=fc.getResult();if(!f.existsAsFile())return;juce::MemoryBlock data;if(f.loadFileAsData(data))processor.setStateInformation(data.getData(),static_cast<int>(data.getSize()));repaint();});}

void SnairCreatorAudioProcessorEditor::captureUndo(){undoState.reset();processor.getStateInformation(undoState);undo.setEnabled(undoState.getSize()>0);}
void SnairCreatorAudioProcessorEditor::restoreUndo(){if(undoState.getSize()==0)return;processor.setStateInformation(undoState.getData(),static_cast<int>(undoState.getSize()));undoState.reset();undo.setEnabled(false);repaint();}

void SnairCreatorAudioProcessorEditor::setParameterActual(const char* id,float actual){if(auto* p=processor.apvts.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(actual));p->endChangeGesture();}}
void SnairCreatorAudioProcessorEditor::applyMacro(int index,float value){switch(index){case 0:setParameterActual("body",value);setParameterActual("crack",.25f+.75f*value);break;case 1:setParameterActual("crack",.35f+.65f*value);setParameterActual("noise",.25f+.75f*value);setParameterActual("clap_spread_ms",35.0f-27.0f*value);break;case 2:setParameterActual("drive_db",18.0f*value);setParameterActual("character",1.0f-.55f*value);break;case 3:setParameterActual("tail_ms",20.0f+1180.0f*value);setParameterActual("output_ms",80.0f+1920.0f*value);break;default:break;}}

void SnairCreatorAudioProcessorEditor::randomizeParameters(bool subtle){captureUndo();juce::Random r;auto set=[this](const char* id,float actual){setParameterActual(id,actual);};if(auto* p=processor.apvts.getParameter("seed"))p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(r.nextInt(0x7fffffff))));const float amount=subtle?.18f:1.0f;auto around=[&](float center,float span){return center+(r.nextFloat()*2-1)*span*amount;};set("body",juce::jlimit(0.0f,1.0f,around(.65f,.3f)));set("crack",juce::jlimit(0.0f,1.0f,around(.7f,.3f)));set("noise",juce::jlimit(0.0f,1.0f,around(.6f,.35f)));set("tail_ms",juce::jlimit(20.0f,1200.0f,around(300,260)));set("clap_spread_ms",juce::jlimit(8.0f,35.0f,around(18,10)));set("width",juce::jlimit(0.0f,1.0f,around(.4f,.45f)));set("drive_db",juce::jlimit(0.0f,18.0f,around(5,6)));set("tone",juce::jlimit(-1.0f,1.0f,around(0,.8f)));set("pitch_st",juce::jlimit(-24.0f,24.0f,around(0,10)));}
void SnairCreatorAudioProcessorEditor::resetParameters(){for(auto* p:processor.getParameters())if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(p))ranged->setValueNotifyingHost(ranged->getDefaultValue());}
void SnairCreatorAudioProcessorEditor::timerCallback(){sourceInfo.setText(processor.getSourceDescription(),juce::dontSendNotification);repaint(waveformBounds);}
