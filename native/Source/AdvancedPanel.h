#pragma once
#include "Parameters.h"
#include "WavExporter.h"
#include <JuceHeader.h>
#include <array>
#include <functional>

class AdvancedPanel final : public juce::Component
{
public:
    AdvancedPanel();

    void setParameters(const SnairParameters&);
    void applyTo(SnairParameters&) const;
    void setMode(SnairMode);
    WavExportOptions exportOptions(float outputTrimDb) const;
    void resized() override;

    std::function<void(bool renderRequired)> onParameterCommit;

private:
    static constexpr int parameterCount=13;
    void configureSlider(int index,const juce::String& name,double min,double max,double step);
    void notify(bool renderRequired);

    juce::Viewport viewport;
    juce::Component content;
    std::array<juce::Label,parameterCount> labels;
    std::array<juce::Slider,parameterCount> sliders;
    juce::ToggleButton normalizeRender{"Normalize render"};

    juce::Label exportLabel;
    juce::ComboBox bitDepth;
    juce::ComboBox sampleRate;
    juce::ComboBox channels;
    juce::ToggleButton exportNormalize{"Normalize export"};
    juce::ToggleButton includeTrim{"Include output trim"};
};
