#pragma once

#include "Pitch.h"
#include "Scale.h"
#include <array>
#include <cstdint>

namespace juce { class String; }

namespace morph
{

enum class ChordQuality
{
    major,
    minor,
    diminished,
    halfDiminished,
    augmented,
    sus2,
    sus4,
    dominant
};

enum ChordExtension : uint16_t
{
    extNone     = 0,
    extFlat7    = 1 << 0,
    extMajor7   = 1 << 1,
    extNinth    = 1 << 2,
    extFlat9    = 1 << 3,
    extSharp9   = 1 << 4,
    extEleventh = 1 << 5,
    extSharp11  = 1 << 6,
    extFlat13   = 1 << 7,
    ext13th     = 1 << 8
};

using ChordExtensionSet = uint16_t;

constexpr bool hasExtension (ChordExtensionSet set, ChordExtension ext)
{
    return (set & (uint16_t) ext) != 0;
}

/** Chord identity as a quality + extensions, compiled to semitone offsets. */
struct ChordFormula
{
    static constexpr int maxTones = 8;

    ChordQuality quality = ChordQuality::minor;
    ChordExtensionSet extensions = extNone;

    std::array<int, maxTones> semitones {}; // ascending from root, includes 0
    int toneCount = 0;

    bool operator== (const ChordFormula&) const = default;

    static ChordFormula make (ChordQuality q, ChordExtensionSet ext);
};

/** Absolute chord: root + formula. */
struct ChordSymbol
{
    PitchClass root { 0 };
    ChordFormula formula;

    bool operator== (const ChordSymbol&) const = default;
};

/** Functional identity: degree on the major diatonic lattice + accidental. */
struct RomanFunction
{
    int degree = 1;       // 1..7
    int accidental = 0;   // -1 flat, 0 natural, +1 sharp
    ChordQuality quality = ChordQuality::minor;
    ChordExtensionSet extensions = extNone;

    bool operator== (const RomanFunction&) const = default;
};

/** A chord choice with its functional reading. */
struct ChordCandidate
{
    ChordSymbol chord;
    RomanFunction roman;
};

/** Display helpers (UI-safe strings, e.g. "Cm9", "bVII", "i"). */
juce::String romanFunctionToString (RomanFunction fn);
juce::String chordSymbolToString (ChordSymbol chord, bool useFlats);

} // namespace morph
