#include "WaveformView.h"
#include <algorithm>

void WaveformView::setSource(std::vector<float> peaks, float transient, float a, float b)
{
    sourcePeaks = std::move(peaks);
    transientPos = transient; attackA = a; attackB = b;
    notice.clear();
    repaint();
}

void WaveformView::setHit(std::vector<float> peaks) { hitPeaks = std::move(peaks); repaint(); }

void WaveformView::clear()
{
    sourcePeaks.clear(); hitPeaks.clear();
    transientPos = attackA = attackB = -1.0f;
    repaint();
}

void WaveformView::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff11161d));
    g.fillRoundedRectangle(bounds, 7.0f);
    g.setColour(juce::Colour(0xff2a303a));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, 1.0f);

    auto inner = bounds.reduced(8.0f, 6.0f);
    if (inner.isEmpty()) return;
    const auto& peaks = showHit ? hitPeaks : sourcePeaks;
    g.setColour(juce::Colour(0xff2a303a));
    g.drawHorizontalLine(static_cast<int>(inner.getCentreY()), inner.getX(), inner.getRight());

    if (peaks.empty())
    {
        g.setColour(juce::Colour(0xff8a929e));
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(notice.isNotEmpty() ? notice : (showHit ? juce::String("NO GENERATED HIT YET") : juce::String("DROP A WAV / AIFF SOURCE HERE")),
                         inner.toNearestInt(), juce::Justification::centred, 2);
        return;
    }

    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8a929e));
    g.drawText(showHit ? "GENERATED HIT  -  DRAG ME INTO YOUR DAW" : "SOURCE  -  magenta line: strongest transient, shaded: attack region",
               inner.removeFromTop(14).toNearestInt(), juce::Justification::topLeft);
    const float cy = inner.getCentreY();
    if (!showHit && attackA >= 0.0f && attackB > attackA)
    {
        g.setColour(juce::Colour(0xffe14ab8).withAlpha(0.16f));
        const float x0 = inner.getX() + inner.getWidth() * attackA;
        g.fillRect(juce::Rectangle<float>(x0, inner.getY(), std::max(2.0f, inner.getWidth() * (attackB - attackA)), inner.getHeight()));
    }

    const float maxPeak = std::max(0.05f, *std::max_element(peaks.begin(), peaks.end()));
    const float half = inner.getHeight() * 0.46f;
    g.setColour(showHit ? juce::Colour(0xff7bc96f) : juce::Colour(0xff2fcae0));
    const float denom = static_cast<float>(std::max<size_t>(1, peaks.size() - 1));
    juce::Path path;
    for (size_t i = 0; i < peaks.size(); ++i)
    {
        const float x = inner.getX() + inner.getWidth() * (static_cast<float>(i) / denom);
        const float a = std::clamp(peaks[i] / maxPeak, 0.0f, 1.0f) * half;
        path.addRectangle(x, cy - a, std::max(1.0f, inner.getWidth() / denom), 2.0f * a + 0.5f);
    }
    g.fillPath(path);

    if (!showHit && transientPos >= 0.0f)
    {
        g.setColour(juce::Colour(0xffe14ab8));
        g.drawVerticalLine(static_cast<int>(inner.getX() + inner.getWidth() * transientPos), inner.getY(), inner.getBottom());
    }
}

void WaveformView::mouseDrag(const juce::MouseEvent& e)
{
    if (showHit && !hitPeaks.empty() && !dragStarted && e.getDistanceFromDragStart() > 6)
    {
        dragStarted = true;
        if (onDragOut) onDragOut();
    }
}

void WaveformView::mouseUp(const juce::MouseEvent& e)
{
    if (!dragStarted && !e.mouseWasDraggedSinceMouseDown() && onClicked) onClicked();
    dragStarted = false;
}
