#pragma once

#include "Scale.h"
#include <optional>

namespace juce { class String; }

namespace morph
{

/** The current tonal context: tonic + scale. Default: C natural minor. */
struct KeyContext
{
    PitchClass tonic { 0 };
    ScaleDefinition scale = ScaleDefinition::naturalMinor();

    /** Pitch class of a 1-based scale degree. */
    constexpr PitchClass pitchClassAtDegree (ScaleDegree degree) const
    {
        return transpose (tonic, Interval { scale.intervalAt (degree) });
    }

    constexpr bool contains (PitchClass pc) const
    {
        for (int d = 1; d <= scale.size; ++d)
            if (pitchClassAtDegree (ScaleDegree { d }) == pc)
                return true;
        return false;
    }

    /** Scale degree for a diatonic pitch class, or nullopt if chromatic. */
    constexpr std::optional<ScaleDegree> degreeOf (PitchClass pc) const
    {
        for (int d = 1; d <= scale.size; ++d)
            if (pitchClassAtDegree (ScaleDegree { d }) == pc)
                return ScaleDegree { d };
        return std::nullopt;
    }

    /** Flat keys prefer flat note spelling. */
    constexpr bool prefersFlats() const
    {
        if (scale.kind == ScaleKind::naturalMinor)
        {
            switch (tonic.value)
            {
                case 0:  case 2:  case 5:  case 7:  case 10: return true;  // C D F G Bb minor
                default: return false;
            }
        }
        return false;
    }

    static constexpr KeyContext cMinor()
    {
        return KeyContext { PitchClass { 0 }, ScaleDefinition::naturalMinor() };
    }
};

/** Header KEY / SCALE control support: 12 minor keys in chromatic order. */
KeyContext keyContextForMinorTonicIndex (int index);
int minorTonicIndexOf (const KeyContext& key);
juce::String keyContextDisplayName (const KeyContext& key);

} // namespace morph
