#include "../TestHarness.h"
#include "ui/keyboard/KeyboardGeometry.h"

using namespace morph;

MORPH_TEST (keyboard, accidentalPitchClasses)
{
    // Spec §90: accidental classes == {1,3,6,8,10}
    for (int pc = 0; pc < 12; ++pc)
    {
        const bool expected = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
        CHECK_EQ (isAccidentalPitchClass (pc), expected);
    }
}

MORPH_TEST (keyboard, noBlackBetweenEForBC)
{
    const auto layout = buildKeyboardLayout (24, 96);

    for (int i = 0; i < layout.count; ++i)
    {
        const auto& k = layout.keys[(size_t) i];
        if (! k.isBlack)
            continue;

        // A black key must sit between two naturals a semitone apart only
        // where a natural semitone gap does NOT exist: never E/F or B/C.
        const int pc = k.midi % 12;
        CHECK (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
    }
}

MORPH_TEST (keyboard, twoThreeGrouping)
{
    const auto layout = buildKeyboardLayout (36, 96); // C2..C7

    // For each full octave: exactly 5 black keys, grouped 2 + 3.
    for (int octave = 0; octave < 5; ++octave)
    {
        const int c = 36 + octave * 12;
        int blackCount = 0;
        int firstGroup = 0, secondGroup = 0;

        for (int i = 0; i < layout.count; ++i)
        {
            const auto& k = layout.keys[(size_t) i];
            if (! k.isBlack || k.midi < c || k.midi >= c + 12)
                continue;

            ++blackCount;
            const int pc = k.midi % 12;
            if (pc == 1 || pc == 3)
                ++firstGroup;
            else
                ++secondGroup;
        }

        CHECK_EQ (blackCount, 5);
        CHECK_EQ (firstGroup, 2);   // C# D#
        CHECK_EQ (secondGroup, 3);  // F# G# A#
    }
}

MORPH_TEST (keyboard, midiOrderingAndCoverage)
{
    const auto layout = buildKeyboardLayout (24, 96);

    int prev = -1;
    int naturalCount = 0;
    for (int i = 0; i < layout.count; ++i)
    {
        CHECK (layout.keys[(size_t) i].midi > prev);
        prev = layout.keys[(size_t) i].midi;
        if (! layout.keys[(size_t) i].isBlack)
            ++naturalCount;
    }

    CHECK_EQ (layout.count, 73);      // 24..96 inclusive
    CHECK_EQ (naturalCount, layout.whiteCount);
    CHECK (layout.totalWidthUnits > 0.0f);
}

MORPH_TEST (keyboard, exactPitchHighlighting)
{
    // Spec §36/§90: exact generated MIDI pitches light up — not pitch classes.
    const auto layout = buildKeyboardLayout (24, 96);

    MusicalPlaybackState state;
    state.innerVoices.fill (-1);
    // Golden Cm9 realization: C2 G2 Bb3 D4 Eb4
    for (int p : { 36, 43, 58, 62, 63 })
        state.currentlySoundingNotes.set ((size_t) p);
    state.generatedRoles[36] = (int8_t) VoiceRole::bass;
    state.generatedRoles[43] = (int8_t) VoiceRole::inner;
    state.generatedRoles[58] = (int8_t) VoiceRole::inner;
    state.generatedRoles[62] = (int8_t) VoiceRole::inner;
    state.generatedRoles[63] = (int8_t) VoiceRole::top;

    const auto lit = computeLitKeys (layout, state);

    CHECK_EQ (lit.count, 5);
    CHECK (lit.find (36) != nullptr);
    CHECK (lit.find (43) != nullptr);
    CHECK (lit.find (58) != nullptr);
    CHECK (lit.find (62) != nullptr);
    CHECK (lit.find (63) != nullptr);

    // No other octave's C/G/Bb/D/Eb may light.
    for (int p : { 24, 48, 60, 72, 84, 31, 55, 67 })
        CHECK (lit.find (p) == nullptr);

    // Semantic role colors (§37): bass = C2, top = Eb4.
    CHECK (lit.find (36)->role == VoiceRole::bass);
    CHECK (lit.find (63)->role == VoiceRole::top);
    CHECK (lit.find (43)->role == VoiceRole::inner);
}
