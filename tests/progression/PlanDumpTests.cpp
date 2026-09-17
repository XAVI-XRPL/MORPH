#include "../TestHarness.h"
#include "engine/EngineHost.h"

using namespace morph;

/**
 * M3 acceptance guard (§97): the gold-family plan must be melodic —
 * the bass moves by step (never parked, never leaping wildly) and the
 * top line forms a controlled contour. Dumps the plan for inspection.
 */
MORPH_TEST (planDump, melodicBassAndTopContour)
{
    EngineHost host;
    host.prepare (48000.0, 512);

    std::array<ChordRealization, 4> plan;
    host.buildProgressionPlan (plan);

    for (int i = 0; i < 4; ++i)
    {
        const auto& r = plan[(size_t) i];
        juce::String line = chordSymbolToString (r.candidate.chord, true).paddedRight (' ', 8);
        line += romanFunctionToString (r.candidate.roman).paddedRight (' ', 6);
        for (int v = 0; v < r.voicing.count; ++v)
        {
            const auto& voice = r.voicing.voices[(size_t) v];
            line += juce::String (voice.pitch.value)
                  + (voice.role == VoiceRole::bass ? "b " : voice.role == VoiceRole::top ? "t " : "  ");
        }
        juce::Logger::writeToLog ("PLAN " + line);
    }

    for (int i = 0; i < 4; ++i)
    {
        const int next = (i + 1) % 4;
        const int bassMove = std::abs (plan[(size_t) next].bassPitch.value
                                       - plan[(size_t) i].bassPitch.value);
        const int topMove = std::abs (plan[(size_t) next].topPitch.value
                                      - plan[(size_t) i].topPitch.value);

        CHECK (bassMove > 0);    // bass never parks between functions
        CHECK (bassMove <= 7);   // stepwise-leaning
        CHECK (topMove <= 5);    // controlled top leaps only
    }
}
