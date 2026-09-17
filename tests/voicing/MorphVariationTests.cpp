#include "../TestHarness.h"
#include "engine/harmony/HarmonyEngine.h"
#include "engine/voicing/VoicingEngine.h"

using namespace morph;

MORPH_TEST (voicing, morphVariationPreservesIdentity)
{
    // MORPH (voicing-level, pre-M5): same chord identity, different shape.
    const auto key = KeyContext::cMinor();
    const auto style = StyleProfile::modernRnB();
    HarmonyEngine harmony;
    VoicingEngine voicing;

    const auto candidate = harmony.chordForDegree (key, ScaleDegree { 1 }, style, 0.5f);

    VoicingContext ctx;
    ctx.openness = 0.4f;
    const auto canonical = voicing.realize (candidate, key, style, ctx, 0);
    const auto sibling1 = voicing.realize (candidate, key, style, ctx, 1);
    const auto sibling2 = voicing.realize (candidate, key, style, ctx, 2);

    // Identity preserved.
    CHECK (sibling1.candidate.chord == canonical.candidate.chord);
    CHECK (sibling2.candidate.chord == canonical.candidate.chord);
    CHECK (chordSymbolToString (sibling1.candidate.chord, true) == "Cm9");
    CHECK_EQ (sibling1.bassPitch.value, canonical.bassPitch.value); // bass DNA kept

    // Same pitch-class content (a sibling, not a new chord).
    auto pitchClasses = [] (const ChordRealization& r)
    {
        std::array<int, 12> pcs {};
        for (int i = 0; i < r.voicing.count; ++i)
            pcs[(size_t) pitchClassOf (r.voicing.voices[(size_t) i].pitch).value] = 1;
        return pcs;
    };
    CHECK (pitchClasses (sibling1) == pitchClasses (canonical));
    CHECK (pitchClasses (sibling2) == pitchClasses (canonical));

    // But the shape changed.
    bool differs1 = sibling1.topPitch != canonical.topPitch;
    bool differs2 = sibling2.topPitch != canonical.topPitch;
    CHECK (differs1);
    CHECK (differs2);

    // Deterministic: same inputs → identical sibling.
    const auto again = voicing.realize (candidate, key, style, ctx, 1);
    CHECK (again.topPitch == sibling1.topPitch);
    CHECK (again.bassPitch == sibling1.bassPitch);
}
