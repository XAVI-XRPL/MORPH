#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <vector>

namespace testfw
{

struct TestCase
{
    juce::String suite;
    juce::String name;
    std::function<void()> fn;
};

std::vector<TestCase>& registry();

struct Registrar
{
    Registrar (const char* suite, const char* name, std::function<void()> fn)
    {
        registry().push_back ({ suite, name, std::move (fn) });
    }
};

struct RunContext
{
    int checks = 0;
    int failures = 0;
};

extern thread_local RunContext* current;

void reportCheck (bool ok, const juce::String& expr, const char* file, int line);

int runAll();

} // namespace testfw

#define MORPH_TEST(suite, name) \
    static void suite##_##name(); \
    static testfw::Registrar registrar_##suite##_##name (#suite, #name, suite##_##name); \
    static void suite##_##name()

#define CHECK(cond) \
    testfw::reportCheck ((cond), juce::String (#cond), __FILE__, __LINE__)

#define CHECK_EQ(a, b) \
    testfw::reportCheck (((a) == (b)), \
        juce::String (#a " == " #b "  (") + juce::String ((int64_t) (a)) \
            + " vs " + juce::String ((int64_t) (b)) + ")", __FILE__, __LINE__)
