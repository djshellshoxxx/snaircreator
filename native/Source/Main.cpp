#include "MainComponent.h"

class SnairCreatorWindow final : public juce::DocumentWindow
{
public:
    explicit SnairCreatorWindow(const juce::String& name)
        : juce::DocumentWindow(name,
                               juce::Colour::fromRGB(15, 18, 23),
                               juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setResizeLimits(760, 520, 1800, 1200);
        setContentOwned(new MainComponent(), true);
        centreWithSize(1000, 700);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class SnairCreatorApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "SnairCreator"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<SnairCreatorWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

private:
    std::unique_ptr<SnairCreatorWindow> mainWindow;
};

START_JUCE_APPLICATION(SnairCreatorApplication)
