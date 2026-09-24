#include "HarmonyEngine.h"

namespace morph
{

ChordCandidate HarmonyEngine::chordForDegree (const KeyContext& key,
                                              ScaleDegree degree,
                                              const StyleProfile& style,
                                              float colorAmount) const
{
    // The style owns the degree → chord vocabulary (M6).
    const auto& spec = style.degrees[(degree.value - 1) % 7];

    auto extensions = spec.extensions;

    // COLOR: harmonic brightness, stylistically contained.
    if (colorAmount < 0.33f)
    {
        extensions &= ~ (ChordExtensionSet) (extNinth | extEleventh); // darker: plain sevenths
    }
    else if (colorAmount > 0.66f)
    {
        if (spec.quality == ChordQuality::minor)
            extensions |= extEleventh;                                 // brighter: add 11 on minors
    }

    const auto root = key.pitchClassAtDegree (degree);

    ChordCandidate result;
    result.chord = ChordSymbol { root, ChordFormula::make (spec.quality, extensions) };
    result.roman = RomanFunction { spec.latticeDegree, spec.accidental, spec.quality, extensions };
    return result;
}

} // namespace morph
