#include "TestHarness.h"

namespace testfw
{

std::vector<TestCase>& registry()
{
    static std::vector<TestCase> r;
    return r;
}

thread_local RunContext* current = nullptr;

void reportCheck (bool ok, const juce::String& expr, const char* file, int line)
{
    if (current == nullptr)
        return;

    ++current->checks;
    if (! ok)
    {
        ++current->failures;
        juce::Logger::writeToLog ("    FAIL: " + juce::String (file) + ":" + juce::String (line)
                                  + "  CHECK(" + expr + ")");
    }
}

int runAll()
{
    int failedSuites = 0;
    int totalChecks = 0;
    int passed = 0;

    for (auto& tc : registry())
    {
        RunContext ctx;
        current = &ctx;

        try
        {
            tc.fn();
        }
        catch (...)
        {
            ++ctx.failures;
            juce::Logger::writeToLog ("    FAIL: uncaught exception");
        }

        current = nullptr;
        totalChecks += ctx.checks;

        if (ctx.failures == 0)
        {
            ++passed;
            juce::Logger::writeToLog ("  PASS  " + tc.suite + "." + tc.name
                                      + "  (" + juce::String (ctx.checks) + " checks)");
        }
        else
        {
            ++failedSuites;
            juce::Logger::writeToLog ("  FAIL  " + tc.suite + "." + tc.name
                                      + "  (" + juce::String (ctx.failures) + " failed)");
        }
    }

    juce::Logger::writeToLog ("");
    juce::Logger::writeToLog (juce::String (passed) + " / " + juce::String ((int) registry().size())
                              + " tests passed, " + juce::String (totalChecks) + " checks");
    return failedSuites == 0 ? 0 : 1;
}

} // namespace testfw

int main (int, char**)
{
    juce::Logger::writeToLog ("MORPH test suite");
    return testfw::runAll();
}
