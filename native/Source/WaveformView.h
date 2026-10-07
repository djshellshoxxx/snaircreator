#pragma once
#include <JuceHeader.h>
#include <functional>
#include <vector>

// Display-only waveform. Shows the source (with strongest transient + selected attack region)
// or the generated hit. In hit view, dragging the waveform exports the hit as a WAV to drop into a DAW.
class WaveformView final : public juce::Component, public juce::SettableTooltipClient
{
public:
    void setSource(std::vector<float> peaks, float transient, float attackStart, float attackEnd);
    void setHit(std::vector<float> peaks);
    void clear();
    void setShowHit(bool shouldShowHit) { showHit = shouldShowHit; repaint(); }
    bool isShowingHit() const noexcept { return showHit; }
    void setMissingSourceNotice(const juce::String& text) { notice = text; repaint(); }

    std::function<void()> onDragOut;  // called when the user drags the generated hit out
    std::function<void()> onClicked;

    void paint(juce::Graphics&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    std::vector<float> sourcePeaks, hitPeaks;
    float transientPos = -1.0f, attackA = -1.0f, attackB = -1.0f;
    bool showHit = false, dragStarted = false;
    juce::String notice;
};
