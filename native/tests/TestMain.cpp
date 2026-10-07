#include <JuceHeader.h>
int main()
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests(0x534E414952);
    int failures=0;
    for(int i=0;i<runner.getNumResults();++i)
        if(const auto* r=runner.getResult(i)) failures += r->failures;
    return runner.getNumResults()>0 && failures==0 ? 0 : 1;
}
