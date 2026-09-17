#pragma once

#include <cstdint>
#include <compare>

namespace morph
{

/** Musical interval in semitones. */
struct Interval
{
    int semitones = 0;

    constexpr auto operator<=> (const Interval&) const = default;
};

namespace intervals
{
    inline constexpr Interval unison          { 0 };
    inline constexpr Interval minorSecond     { 1 };
    inline constexpr Interval majorSecond     { 2 };
    inline constexpr Interval minorThird      { 3 };
    inline constexpr Interval majorThird      { 4 };
    inline constexpr Interval perfectFourth   { 5 };
    inline constexpr Interval tritone         { 6 };
    inline constexpr Interval perfectFifth    { 7 };
    inline constexpr Interval minorSixth      { 8 };
    inline constexpr Interval majorSixth      { 9 };
    inline constexpr Interval minorSeventh    { 10 };
    inline constexpr Interval majorSeventh    { 11 };
    inline constexpr Interval octave          { 12 };
}

/** Pitch class 0..11 (C = 0). No octave information. */
struct PitchClass
{
    uint8_t value = 0; // invariant: < 12

    constexpr PitchClass() = default;
    explicit constexpr PitchClass (int v) : value ((uint8_t) (((v % 12) + 12) % 12)) {}

    constexpr auto operator<=> (const PitchClass&) const = default;
};

constexpr PitchClass transpose (PitchClass pc, Interval iv)
{
    return PitchClass (pc.value + iv.semitones);
}

/** Upward interval from a to b (0..11 semitones). */
constexpr Interval intervalBetween (PitchClass a, PitchClass b)
{
    return Interval { ((int) b.value - (int) a.value + 12) % 12 };
}

/** Absolute MIDI pitch 0..127. Middle C (C4) = 60. */
struct MidiPitch
{
    int value = 60;

    constexpr auto operator<=> (const MidiPitch&) const = default;

    constexpr bool isValid() const { return value >= 0 && value <= 127; }
};

constexpr MidiPitch transpose (MidiPitch p, Interval iv)
{
    return MidiPitch { p.value + iv.semitones };
}

constexpr PitchClass pitchClassOf (MidiPitch p)
{
    return PitchClass (p.value);
}

/** Octave number using the C4 = 60 convention (C2 = 36, C3 = 48). */
constexpr int octaveOf (MidiPitch p)
{
    return p.value / 12 - 1;
}

/** Builds a MIDI pitch from pitch class + octave (C4 = 60 convention). */
constexpr MidiPitch makePitch (PitchClass pc, int octave)
{
    return MidiPitch { (octave + 1) * 12 + pc.value };
}

constexpr bool isNaturalPitchClass (int pitchClass)
{
    switch (((pitchClass % 12) + 12) % 12)
    {
        case 0: case 2: case 4: case 5: case 7: case 9: case 11: return true;
        default: return false;
    }
}

constexpr bool isAccidentalPitchClass (int pitchClass)
{
    return ! isNaturalPitchClass (pitchClass);
}

/** Display name for a pitch class ("C", "Bb", "F#"). Flats for minor/flat keys. */
const char* pitchClassName (PitchClass pc, bool useFlats);

} // namespace morph
