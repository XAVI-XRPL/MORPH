#include "../TestHarness.h"
#include "engine/harmony/HarmonyEngine.h"
#include "engine/voicing/VoicingEngine.h"
#include "engine/progression/Progression.h"

using namespace morph;

namespace
{
    ChordCandidate candidateFor (int degree)
    {
        const auto key = KeyContext::cMinor();
        const auto style = StyleProfile::modernRnB();
        HarmonyEngine harmony;
        return harmony.chordForDegree (key, ScaleDegree { degree }, style, 0.5f);
    }

    VoicingContext contextFrom (const ChordRealization& prev, int slot)
    {
        VoicingContext ctx;
        ctx.previous = &prev.voicing;
        ctx.hasPreviousBass = true;
        ctx.previousBass = prev.bassPitch;
        ctx.hasPreviousTop = true;
        ctx.previousTop = prev.topPitch;
        ctx.slotIndex = slot;
        ctx.motion = 0.5f;
        ctx.openness = 0.4f;
        return ctx;
    }

    int totalVoiceMovement (const Voicing& a, const Voicing& b)
    {
        // Greedy nearest-neighbor distance sum (same metric family as scorer).
        int total = 0;
        for (int i = 0; i < a.count; ++i)
        {
            int dmin = 24;
            for (int j = 0; j < b.count; ++j)
                dmin = std::min (dmin, std::abs (a.voices[(size_t) i].pitch.value
                                               - b.voices[(size_t) j].pitch.value));
            total += std::min (dmin, 12);
        }
        return total;
    }
}

MORPH_TEST (voiceLeading, secondChordMovesLessThanNaive)
{
    VoicingEngine voicing;
    const auto cm9 = candidateFor (1);
    const auto abmaj9 = candidateFor (6);

    // Canonical first chord (no context).
    VoicingContext empty;
    empty.openness = 0.4f;
    const auto first = voicing.realize (cm9, KeyContext::cMinor(),
                                        StyleProfile::modernRnB(), empty, 0);

    // Context-led second chord vs naive recipe with the same top rule.
    const auto led = voicing.realize (abmaj9, KeyContext::cMinor(),
                                      StyleProfile::modernRnB(), contextFrom (first, 1), 0);

    VoicingContext naiveEmpty;
    naiveEmpty.openness = 0.4f;
    const auto naive = voicing.realize (abmaj9, KeyContext::cMinor(),
                                        StyleProfile::modernRnB(), naiveEmpty, 0);

    const int ledMove = totalVoiceMovement (led.voicing, first.voicing);
    const int naiveMove = totalVoiceMovement (naive.voicing, first.voicing);

    CHECK (ledMove < naiveMove);
}

MORPH_TEST (voiceLeading, progressionIsConnected)
{
    // M3 acceptance: the gold family should play as connected voices —
    // bounded per-chord movement, stepwise-leaning top line.
    VoicingEngine voicing;
    const Progression prog;

    ChordRealization prev;
    bool hasPrev = false;
    int worstMove = 0;
    int worstTopLeap = 0;

    for (int slot = 0; slot < prog.size * 2; ++slot) // run the loop twice
    {
        const int degree = prog.slots[(size_t) (slot % prog.size)].degree.value;
        const auto cand = candidateFor (degree);

        ChordRealization r;
        if (! hasPrev)
        {
            VoicingContext empty;
            empty.openness = 0.4f;
            r = voicing.realize (cand, KeyContext::cMinor(), StyleProfile::modernRnB(), empty, 0);
        }
        else
        {
            r = voicing.realize (cand, KeyContext::cMinor(), StyleProfile::modernRnB(),
                                 contextFrom (prev, slot % prog.size), 0);
            worstMove = std::max (worstMove, totalVoiceMovement (r.voicing, prev.voicing));
            worstTopLeap = std::max (worstTopLeap,
                                     std::abs (r.topPitch.value - prev.topPitch.value));
        }

        // Invariants hold for every chord.
        for (int i = 1; i < r.voicing.count; ++i)
            CHECK (r.voicing.voices[(size_t) i].pitch.value
                   > r.voicing.voices[(size_t) (i - 1)].pitch.value);
        CHECK (r.bassPitch.value >= 36 && r.bassPitch.value <= 47);

        prev = r;
        hasPrev = true;
    }

    // Bounded movement and a melodic top line (no wild leaps).
    CHECK (worstMove <= 26);
    CHECK (worstTopLeap <= 7);
}

MORPH_TEST (voiceLeading, deterministicSequence)
{
    VoicingEngine voicing;
    const Progression prog;

    auto runSequence = [&]
    {
        std::array<int, 8> tops {};
        ChordRealization prev;
        bool hasPrev = false;
        for (int slot = 0; slot < prog.size * 2; ++slot)
        {
            const int degree = prog.slots[(size_t) (slot % prog.size)].degree.value;
            const auto cand = candidateFor (degree);
            ChordRealization r;
            if (! hasPrev)
            {
                VoicingContext empty;
                empty.openness = 0.4f;
                r = voicing.realize (cand, KeyContext::cMinor(), StyleProfile::modernRnB(), empty, 0);
            }
            else
            {
                r = voicing.realize (cand, KeyContext::cMinor(), StyleProfile::modernRnB(),
                                     contextFrom (prev, slot % prog.size), 0);
            }
            tops[(size_t) slot] = r.topPitch.value;
            prev = r;
            hasPrev = true;
        }
        return tops;
    };

    CHECK (runSequence() == runSequence());
}
