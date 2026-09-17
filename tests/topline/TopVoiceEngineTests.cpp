#include "../TestHarness.h"
#include "engine/topline/TopVoiceEngine.h"

using namespace morph;

namespace
{
    ChordSymbol chordOf (int rootPc, ChordQuality q, ChordExtensionSet ext)
    {
        return ChordSymbol { PitchClass { rootPc }, ChordFormula::make (q, ext) };
    }
}

MORPH_TEST (topline, stepwisePreferredOverLeaps)
{
    TopVoiceEngine top;
    TopVoiceContext ctx;
    ctx.hasPrevious = true;
    ctx.previousTop = MidiPitch { 63 }; // Eb4
    ctx.motion = 0.5f;

    // Fm9 tones near Eb4: Ab4(68), C5(72), Eb5(75), F4(65), G4(67)...
    // Stepwise targets should beat distant ones.
    const auto chosen = top.chooseTop (chordOf (5, ChordQuality::minor, extFlat7 | extNinth), ctx);
    CHECK (std::abs (chosen.value - 63) <= 5);
}

MORPH_TEST (topline, repetitionAnchorsMotif)
{
    TopVoiceEngine top;
    TopVoiceContext ctx;
    ctx.hasPrevious = true;
    ctx.previousTop = MidiPitch { 65 }; // F4
    ctx.motion = 0.2f; // still: repetition rewarded

    // Chord containing F4 (e.g. Bb9: Bb C D F Ab) — F5=77 out, F4=65 in range.
    const auto chosen = top.chooseTop (chordOf (10, ChordQuality::dominant, extNinth), ctx);
    CHECK_EQ (chosen.value, 65);
}

MORPH_TEST (topline, registerBounds)
{
    TopVoiceEngine top;
    TopVoiceContext ctx;

    for (int root = 0; root < 12; ++root)
    {
        const auto chosen = top.chooseTop (chordOf (root, ChordQuality::minor, extFlat7 | extNinth), ctx);
        CHECK (chosen.value >= TopVoiceEngine::registerLow);
        CHECK (chosen.value <= TopVoiceEngine::registerHigh);
    }
}

MORPH_TEST (topline, neverJustHighestChordTone)
{
    // §51: the top voice is melodic — a chord whose highest candidate would
    // leap wildly from the previous top must pick a nearer tone instead.
    TopVoiceEngine top;
    TopVoiceContext ctx;
    ctx.hasPrevious = true;
    ctx.previousTop = MidiPitch { 60 }; // C4
    ctx.motion = 0.5f;

    const auto chord = chordOf (8, ChordQuality::major, extMajor7 | extNinth); // Abmaj9
    const auto cands = TopVoiceEngine::candidatesFor (chord);
    const auto chosen = top.chooseTop (chord, ctx);

    CHECK (chosen.value != cands.pitches[(size_t) (cands.count - 1)]); // not the highest
    CHECK (std::abs (chosen.value - 60) <= 5);
}

MORPH_TEST (topline, deterministic)
{
    TopVoiceEngine top;
    TopVoiceContext ctx;
    ctx.hasPrevious = true;
    ctx.previousTop = MidiPitch { 67 };
    ctx.motion = 0.8f;
    ctx.previousDirection = 1;

    const auto a = top.chooseTop (chordOf (7, ChordQuality::sus4, extFlat7), ctx);
    const auto b = top.chooseTop (chordOf (7, ChordQuality::sus4, extFlat7), ctx);
    CHECK_EQ (a.value, b.value);
}
