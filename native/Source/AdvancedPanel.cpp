#include "AdvancedPanel.h"

AdvancedPanel::AdvancedPanel()
{
    viewport.setViewedComponent(&content,false);
    viewport.setScrollBarsShown(true,false);
    addAndMakeVisible(viewport);

    configureSlider(0,"BODY FREQ",70.0,450.0,1.0);
    configureSlider(1,"ATTACK",0.0,1.0,0.01);
    configureSlider(2,"NOISE",0.0,1.0,0.01);
    configureSlider(3,"TAIL MS",20.0,2000.0,1.0);
    configureSlider(4,"PITCH ST",-24.0,24.0,0.1);
    configureSlider(5,"TONE",-1.0,1.0,0.01);
    configureSlider(6,"DRIVE DB",0.0,24.0,0.1);
    configureSlider(7,"WIDTH",0.0,1.0,0.01);
    configureSlider(8,"CLAP COUNT",2.0,6.0,1.0);
    configureSlider(9,"CLAP SPREAD",8.0,35.0,0.1);
    configureSlider(10,"CROSS BLEND",0.0,1.0,0.01);
    configureSlider(11,"OUTPUT TRIM",-24.0,12.0,0.1);
    configureSlider(12,"SEED",0.0,2147483647.0,1.0);

    normalizeRender.setTooltip("Normalize the generated hit before it becomes the active render.");
    normalizeRender.onClick=[this]{notify(true);};
    content.addAndMakeVisible(normalizeRender);

    exportLabel.setText("EXPORT OPTIONS",juce::dontSendNotification);
    exportLabel.setColour(juce::Label::textColourId,juce::Colour::fromRGB(47,202,224));
    content.addAndMakeVisible(exportLabel);

    bitDepth.addItem("16-bit PCM",16);
    bitDepth.addItem("24-bit PCM",24);
    bitDepth.addItem("32-bit float",32);
    bitDepth.setSelectedId(24,juce::dontSendNotification);
    bitDepth.setTooltip("WAV sample format.");
    content.addAndMakeVisible(bitDepth);

    sampleRate.addItem("Current rate",1);
    sampleRate.addItem("44.1 kHz",2);
    sampleRate.addItem("48 kHz",3);
    sampleRate.addItem("88.2 kHz",4);
    sampleRate.addItem("96 kHz",5);
    sampleRate.setSelectedId(1,juce::dontSendNotification);
    sampleRate.setTooltip("WAV export sample rate.");
    content.addAndMakeVisible(sampleRate);

    channels.addItem("Mono",1);
    channels.addItem("Stereo",2);
    channels.setSelectedId(2,juce::dontSendNotification);
    channels.setTooltip("WAV export channel layout.");
    content.addAndMakeVisible(channels);

    exportNormalize.setTooltip("Normalize a copy during export without changing the active render.");
    includeTrim.setTooltip("Apply the current output trim to the exported WAV.");
    includeTrim.setToggleState(true,juce::dontSendNotification);
    content.addAndMakeVisible(exportNormalize);
    content.addAndMakeVisible(includeTrim);
}

void AdvancedPanel::configureSlider(int index,const juce::String& name,double min,double max,double step)
{
    auto& label=labels[(size_t)index];
    label.setText(name,juce::dontSendNotification);
    label.setColour(juce::Label::textColourId,juce::Colour::fromRGB(145,157,171));
    label.setFont(juce::Font(10.0f,juce::Font::bold));
    content.addAndMakeVisible(label);

    auto& slider=sliders[(size_t)index];
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setRange(min,max,step);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,76,22);
    slider.setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
    slider.setTooltip(name);
    slider.onDragEnd=[this,index]{notify(index!=11);};
    content.addAndMakeVisible(slider);
}

void AdvancedPanel::notify(bool renderRequired)
{
    if(onParameterCommit) onParameterCommit(renderRequired);
}

void AdvancedPanel::setParameters(const SnairParameters& p)
{
    const double values[parameterCount]={
        p.bodyFreqHz,p.attack,p.noise,p.tailMs,p.pitchSt,p.tone,p.driveDb,p.width,
        static_cast<double>(p.clapCount),p.clapSpreadMs,p.crossBlend,p.outputTrimDb,
        static_cast<double>(p.seed)
    };
    for(int i=0;i<parameterCount;++i)
        sliders[(size_t)i].setValue(values[i],juce::dontSendNotification);
    normalizeRender.setToggleState(p.normalizeRender,juce::dontSendNotification);
    setMode(p.mode);
}

void AdvancedPanel::applyTo(SnairParameters& p) const
{
    p.bodyFreqHz=(float)sliders[0].getValue();
    p.attack=(float)sliders[1].getValue();
    p.noise=(float)sliders[2].getValue();
    p.tailMs=(float)sliders[3].getValue();
    p.pitchSt=(float)sliders[4].getValue();
    p.tone=(float)sliders[5].getValue();
    p.driveDb=(float)sliders[6].getValue();
    p.width=(float)sliders[7].getValue();
    p.clapCount=(int)sliders[8].getValue();
    p.clapSpreadMs=(float)sliders[9].getValue();
    p.crossBlend=(float)sliders[10].getValue();
    p.outputTrimDb=(float)sliders[11].getValue();
    p.seed=(uint32_t)sliders[12].getValue();
    p.normalizeRender=normalizeRender.getToggleState();
    p.sanitize();
}

void AdvancedPanel::setMode(SnairMode mode)
{
    const bool clap=mode==SnairMode::clap;
    labels[8].setEnabled(clap);
    labels[9].setEnabled(clap);
    sliders[8].setEnabled(clap);
    sliders[9].setEnabled(clap);
}

WavExportOptions AdvancedPanel::exportOptions(float outputTrimDb) const
{
    WavExportOptions result;
    result.bitDepth=bitDepth.getSelectedId();
    result.channels=channels.getSelectedId();
    switch(sampleRate.getSelectedId())
    {
        case 2: result.sampleRate=44100.0; break;
        case 3: result.sampleRate=48000.0; break;
        case 4: result.sampleRate=88200.0; break;
        case 5: result.sampleRate=96000.0; break;
        default: result.sampleRate=0.0; break;
    }
    result.normalize=exportNormalize.getToggleState();
    result.includeOutputTrim=includeTrim.getToggleState();
    result.outputTrimDb=outputTrimDb;
    return result;
}

void AdvancedPanel::resized()
{
    viewport.setBounds(getLocalBounds());
    const int width=std::max(560,viewport.getWidth()-14);
    const int rowHeight=32;
    const int rows=7;
    const int exportHeight=90;
    content.setSize(width,rows*rowHeight+exportHeight);

    const int columnWidth=width/2;
    for(int i=0;i<parameterCount;++i)
    {
        const int column=i%2;
        const int row=i/2;
        auto line=juce::Rectangle<int>(column*columnWidth,row*rowHeight,columnWidth,rowHeight).reduced(4,2);
        labels[(size_t)i].setBounds(line.removeFromLeft(92));
        sliders[(size_t)i].setBounds(line);
    }

    auto normalizeLine=juce::Rectangle<int>(columnWidth,(parameterCount/2)*rowHeight,columnWidth,rowHeight).reduced(4,2);
    normalizeRender.setBounds(normalizeLine);

    auto exportArea=juce::Rectangle<int>(0,rows*rowHeight,width,exportHeight).reduced(4);
    exportLabel.setBounds(exportArea.removeFromTop(20));
    auto comboRow=exportArea.removeFromTop(28);
    bitDepth.setBounds(comboRow.removeFromLeft(140));
    comboRow.removeFromLeft(8);
    sampleRate.setBounds(comboRow.removeFromLeft(140));
    comboRow.removeFromLeft(8);
    channels.setBounds(comboRow.removeFromLeft(110));
    auto toggleRow=exportArea.removeFromTop(28);
    exportNormalize.setBounds(toggleRow.removeFromLeft(160));
    includeTrim.setBounds(toggleRow.removeFromLeft(180));
}
