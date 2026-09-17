#include "../TestHarness.h"
#include "engine/bass/BassEngine.h"

using namespace morph;

namespace
{
    ChordSymbol chordOf (int rootPc, ChordQuality q, ChordExtensionSet ext)
    {
        return ChordSymbol { PitchClass { rootPc }, ChordFormula::make (q, ext) };
    }
}

MORPH_TEST (bass, rootProfileUsesRoot)
{
    BassEngine bass;
    BassContext ctx;
    ctx.motion = 0.0f; // still → root
    ctx.hasPrevious = true;
    ctx.previousBass = MidiPitch { 40 };

    // Even with a closer fifth available, root profile stays on the root.
    const auto b = bass.chooseBass (chordOf (0, ChordQuality::minor, extFlat7 | extNinth), ctx);
    CHECK_EQ (b.value, 36); // C2
}

MORPH_TEST (bass, flowChoosesSmoothestChordTone)
{
    BassEngine bass;
    BassContext ctx;
    ctx.motion = 0.5f; // flow
    ctx.hasPrevious = true;
    ctx.previousBass = MidiPitch { 43 }; // G2

    // Cm9 from G2: parking on G2 (the fifth) would be static under new
    // harmony — flow moves by step to the third, Eb2 (39).
    const auto b = bass.chooseBass (chordOf (0, ChordQuality::minor, extFlat7 | extNinth), ctx);
    CHECK_EQ (b.value, 39); // Eb2 — smooth third-in-the-bass

    // But when the previous bass IS the new chord's root, parking is correct:
    ctx.previousBass = MidiPitch { 36 }; // C2
    const auto stay = bass.chooseBass (chordOf (0, ChordQuality::minor, extFlat7 | extNinth), ctx);
    CHECK_EQ (stay.value, 36); // C2 root holds
}

MORPH_TEST (bass, bounceAlternatesRootAndFifth)
{
    BassEngine bass;
    BassContext ctx;
    ctx.motion = 0.9f; // bounce

    const auto cm9 = chordOf (0, ChordQuality::minor, extFlat7 | extNinth);
    ctx.slotIndex = 0;
    CHECK_EQ (bass.chooseBass (cm9, ctx).value, 36); // C2 (root)
    ctx.slotIndex = 1;
    CHECK_EQ (bass.chooseBass (cm9, ctx).value, 43); // G2 (fifth)
    ctx.slotIndex = 2;
    CHECK_EQ (bass.chooseBass (cm9, ctx).value, 36); // back to root
}

MORPH_TEST (bass, pedalHoldsTonalCenter)
{
    BassEngine bass;
    BassContext ctx;
    ctx.motion = 0.5f;
    ctx.profileOverride = (int) BassMotionProfile::pedal;
    ctx.tonalCenter = PitchClass { 0 }; // C

    // Whatever the chord, the bass holds the tonal center.
    for (int root : { 0, 3, 5, 7, 8 })
    {
        const auto b = bass.chooseBass (chordOf (root, ChordQuality::minor, extFlat7 | extNinth), ctx);
        CHECK_EQ (b.value, 36); // C2 pedal
    }
}

MORPH_TEST (bass, registerDisciplineAcrossKeys)
{
    // §49: bass always lands in [36..47] regardless of root.
    BassEngine bass;
    BassContext ctx;
    for (float motion : { 0.0f, 0.5f, 0.9f })
    {
        ctx.motion = motion;
        for (int root = 0; root < 12; ++root)
        {
            const auto b = bass.chooseBass (chordOf (root, ChordQuality::major, extMajor7 | extNinth), ctx);
            CHECK (b.value >= 36 && b.value <= 47);
        }
    }
}
