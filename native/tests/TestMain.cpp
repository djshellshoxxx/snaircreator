#include <JuceHeader.h>
int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    return runner.getNumResults() > 0 && runner.getNumFailures() == 0 ? 0 : 1;
}
