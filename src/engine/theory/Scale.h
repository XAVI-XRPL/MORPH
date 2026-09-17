#pragma once

#include "Pitch.h"
#include <array>

namespace morph
{

/** Scale degree 1..7 within the diatonic lattice of the current scale. */
struct ScaleDegree
{
    int value = 1; // invariant: 1..7

    constexpr auto operator<=> (const ScaleDegree&) const = default;
};

enum class ScaleKind
{
    naturalMinor,
    major
};

/** A heptatonic scale: semitone offsets from the tonic, ascending. */
struct ScaleDefinition
{
    static constexpr int maxSize = 7;

    std::array<uint8_t, maxSize> intervals {}; // includes 0
    int size = 7;
    ScaleKind kind = ScaleKind::naturalMinor;

    /** Offset in semitones for a 1-based scale degree. */
    constexpr int intervalAt (ScaleDegree degree) const
    {
        return intervals[(size_t) (degree.value - 1) % (size_t) size];
    }

    static constexpr ScaleDefinition naturalMinor()
    {
        ScaleDefinition s;
        s.intervals = { 0, 2, 3, 5, 7, 8, 10 };
        s.size = 7;
        s.kind = ScaleKind::naturalMinor;
        return s;
    }

    static constexpr ScaleDefinition major()
    {
        ScaleDefinition s;
        s.intervals = { 0, 2, 4, 5, 7, 9, 11 };
        s.size = 7;
        s.kind = ScaleKind::major;
        return s;
    }
};

} // namespace morph
