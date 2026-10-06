#include "WaveformView.h"

#include <algorithm>

void WaveformView::setPeaks(std::vector<float> peaks)
{
    peakValues = std::move(peaks);
    repaint();
}

void WaveformView::clear()
{
    peakValues.clear();
    repaint();
}

void WaveformView::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat();
    graphics.setColour(juce::Colour::fromRGB(17, 22, 29));
    graphics.fillRoundedRectangle(bounds, 7.0f);
    graphics.setColour(juce::Colour::fromRGB(48, 58, 70));
    graphics.drawRoundedRectangle(bounds, 7.0f, 1.0f);

    auto inner = bounds.reduced(8.0f, 6.0f);
    if (inner.isEmpty())
        return;

    const auto centreY = inner.getCentreY();
    graphics.setColour(juce::Colour::fromRGB(48, 58, 70));
    graphics.drawHorizontalLine(static_cast<int>(centreY), inner.getX(), inner.getRight());

    if (peakValues.empty())
    {
        graphics.setColour(juce::Colour::fromRGB(145, 157, 171));
        graphics.setFont(juce::Font(11.0f));
        graphics.drawFittedText("WAVEFORM  /  LOAD SOURCE",
                                inner.toNearestInt(),
                                juce::Justification::centred,
                                1);
        return;
    }

    const auto maxPeak = std::max(0.05f, *std::max_element(peakValues.begin(), peakValues.end()));
    const auto halfHeight = inner.getHeight() * 0.46f;
    graphics.setColour(juce::Colour::fromRGB(47, 202, 224));

    for (size_t i = 0; i < peakValues.size(); ++i)
    {
        const auto x = inner.getX() + inner.getWidth()
                     * (static_cast<float>(i) / static_cast<float>(peakValues.size() - 1));
        const auto amplitude = std::clamp(peakValues[i] / maxPeak, 0.0f, 1.0f) * halfHeight;
        graphics.drawVerticalLine(static_cast<int>(x),
                                  centreY - amplitude,
                                  centreY + amplitude);
    }
}
