#include "../TestHarness.h"
#include "engine/EngineHost.h"

using namespace morph;

namespace
{
    constexpr double sr = 48000.0;
}

MORPH_TEST (progressionPlan, deterministicAndValid)
{
    EngineHost host;
    host.prepare (sr, 512);

    std::array<ChordRealization, 4> a, b;
    host.buildProgressionPlan (a);
    host.buildProgressionPlan (b);

    for (int i = 0; i < 4; ++i)
    {
        CHECK (a[(size_t) i].valid);
        CHECK (a[(size_t) i].voicing.count >= 3);
        CHECK (a[(size_t) i].bassPitch == b[(size_t) i].bassPitch);
        CHECK (a[(size_t) i].topPitch == b[(size_t) i].topPitch);

        for (int v = 1; v < a[(size_t) i].voicing.count; ++v)
            CHECK (a[(size_t) i].voicing.voices[(size_t) v].pitch.value
                   > a[(size_t) i].voicing.voices[(size_t) (v - 1)].pitch.value);
    }
}

MORPH_TEST (progressionPlan, goldFamilyIdentities)
{
    EngineHost host;
    host.prepare (sr, 512);

    std::array<ChordRealization, 4> plan;
    host.buildProgressionPlan (plan);

    const char* expected[4] = { "Cm9", "Abmaj9", "Fm9", "G7sus" };
    for (int i = 0; i < 4; ++i)
        CHECK (chordSymbolToString (plan[(size_t) i].candidate.chord, true)
               == juce::String (expected[i]));
}

MORPH_TEST (progressionPlan, exportMatchesPlanPlayback)
{
    // Export renders the same plan the sequencer plays (§79).
    EngineHost host;
    host.prepare (sr, 512);
    host.setTempoBpm (120.0);

    const auto events = host.renderProgressionPerformance();
    CHECK_EQ ((int) events.size(), 4);

    std::array<ChordRealization, 4> plan;
    host.buildProgressionPlan (plan);

    for (size_t i = 0; i < 4; ++i)
    {
        CHECK_EQ (events[i].notes.count, plan[i].voicing.count);
        for (int n = 0; n < events[i].notes.count; ++n)
            CHECK_EQ (events[i].notes.notes[(size_t) n].pitch,
                      plan[i].voicing.voices[(size_t) n].pitch.value);
    }
}
