#include <JuceHeader.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests(0x534E414952);
    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* r = runner.getResult(i)) failures += r->failures;
    std::cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << " (" << runner.getNumResults() << " groups, " << failures << " failures)\n";
    return runner.getNumResults() > 0 && failures == 0 ? 0 : 1;
}
