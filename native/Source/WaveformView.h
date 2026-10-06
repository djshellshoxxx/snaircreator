#pragma once

#include <JuceHeader.h>
#include <vector>

class WaveformView final : public juce::Component
{
public:
    void setPeaks(std::vector<float> peaks);
    void clear();

    void paint(juce::Graphics& graphics) override;

private:
    std::vector<float> peakValues;
};
