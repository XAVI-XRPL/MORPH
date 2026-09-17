#include "../TestHarness.h"
#include "engine/harmony/HarmonyEngine.h"
#include "engine/voicing/VoicingEngine.h"

using namespace morph;

namespace
{
    ChordRealization realizeDegree (int degree, float color = 0.5f, const Voicing* prev = nullptr)
    {
        const auto key = KeyContext::cMinor();
        const auto style = StyleProfile::modernRnB();
        HarmonyEngine harmony;
        VoicingEngine voicing;
        const auto candidate = harmony.chordForDegree (key, ScaleDegree { degree }, style, color);
        VoicingContext ctx;
        ctx.openness = 0.4f;
        if (prev != nullptr && prev->count > 0)
        {
            ctx.previous = prev;
            ctx.hasPreviousBass = true;
            ctx.previousBass = prev->lowest();
            ctx.hasPreviousTop = true;
            ctx.previousTop = prev->highest();
        }
        return voicing.realize (candidate, key, style, ctx, 0);
    }
}

MORPH_TEST (voicing, goldenCm9Realization)
{
    // Spec §105: C minor, Modern R&B, degree 1 → Cm9 voiced
    // C2 G2 Bb3 D4 Eb4, bass C2, top Eb4.
    const auto r = realizeDegree (1);

    CHECK (r.valid);
    CHECK (chordSymbolToString (r.candidate.chord, true) == "Cm9");
    CHECK_EQ (r.voicing.count, 5);

    CHECK_EQ (r.voicing.voices[0].pitch.value, 36); // C2
    CHECK_EQ (r.voicing.voices[1].pitch.value, 43); // G2
    CHECK_EQ (r.voicing.voices[2].pitch.value, 58); // Bb3
    CHECK_EQ (r.voicing.voices[3].pitch.value, 62); // D4
    CHECK_EQ (r.voicing.voices[4].pitch.value, 63); // Eb4

    CHECK_EQ (r.bassPitch.value, 36);
    CHECK_EQ (r.topPitch.value, 63);

    CHECK (r.voicing.voices[0].role == VoiceRole::bass);
    CHECK (r.voicing.voices[1].role == VoiceRole::inner);
    CHECK (r.voicing.voices[2].role == VoiceRole::inner);
    CHECK (r.voicing.voices[3].role == VoiceRole::inner);
    CHECK (r.voicing.voices[4].role == VoiceRole::top);
}

MORPH_TEST (voicing, goldProgressionFamily)
{
    // Spec §106: i9 → bVImaj9 → iv9 → V7sus
    const auto key = KeyContext::cMinor();
    const auto style = StyleProfile::modernRnB();
    HarmonyEngine harmony;
    VoicingEngine voicing;

    Voicing prev {};
    const Voicing* prevPtr = nullptr;
    const char* expected[4] = { "Cm9", "Abmaj9", "Fm9", "G7sus" };

    for (int slot = 0; slot < 4; ++slot)
    {
        const int degree = slot == 0 ? 1 : slot == 1 ? 6 : slot == 2 ? 4 : 5;
        const auto candidate = harmony.chordForDegree (key, ScaleDegree { degree }, style, 0.5f);
        VoicingContext ctx;
        ctx.openness = 0.4f;
        if (prevPtr != nullptr && prevPtr->count > 0)
        {
            ctx.previous = prevPtr;
            ctx.hasPreviousBass = true;
            ctx.previousBass = prevPtr->lowest();
            ctx.hasPreviousTop = true;
            ctx.previousTop = prevPtr->highest();
        }
        const auto r = voicing.realize (candidate, key, style, ctx, 0);

        CHECK (r.valid);
        CHECK (chordSymbolToString (r.candidate.chord, true) == juce::String (expected[slot]));
        CHECK (r.voicing.count >= 4);

        // Ascending order, bass below 48, inner voices above bass+4.
        for (int i = 1; i < r.voicing.count; ++i)
            CHECK (r.voicing.voices[(size_t) i].pitch.value > r.voicing.voices[(size_t) (i - 1)].pitch.value);
        CHECK (r.bassPitch.value >= 36 && r.bassPitch.value <= 47);

        prev = r.voicing;
        prevPtr = &prev;
    }
}

MORPH_TEST (voicing, lowEndCleanliness)
{
    // Spec §49: below MIDI 48 no dense clusters; below 36 nothing but bass.
    for (int degree = 1; degree <= 7; ++degree)
    {
        const auto r = realizeDegree (degree);
        int below48 = 0;
        for (int i = 0; i < r.voicing.count; ++i)
        {
            const int p = r.voicing.voices[(size_t) i].pitch.value;
            CHECK (p >= 36); // nothing below the bass register floor
            if (p < 48)
                ++below48;
        }
        CHECK (below48 <= 2); // bass + at most a fifth
    }
}

MORPH_TEST (voicing, spaceKnobOpensVoicing)
{
    const auto key = KeyContext::cMinor();
    const auto style = StyleProfile::modernRnB();
    HarmonyEngine harmony;
    const auto candidate = harmony.chordForDegree (key, ScaleDegree { 1 }, style, 0.5f);

    VoicingEngine voicing;
    VoicingContext closedCtx; closedCtx.openness = 0.1f;
    VoicingContext openCtx;   openCtx.openness = 0.9f;
    const auto closedR = voicing.realize (candidate, key, style, closedCtx, 0);
    const auto openR = voicing.realize (candidate, key, style, openCtx, 0);

    const int closedSpan = closedR.topPitch.value - closedR.bassPitch.value;
    const int openSpan = openR.topPitch.value - openR.bassPitch.value;
    CHECK (openSpan > closedSpan);
}
