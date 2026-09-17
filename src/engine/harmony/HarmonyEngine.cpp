#include "HarmonyEngine.h"

namespace morph
{

ChordCandidate HarmonyEngine::chordForDegree (const KeyContext& key,
                                              ScaleDegree degree,
                                              const StyleProfile& style,
                                              float colorAmount) const
{
    (void) style;

    // Modern R&B natural-minor functional vocabulary.
    // Roman numerals are expressed on the major lattice (degree 3/6/7 in
    // natural minor carry a flat accidental: bIII, bVI, bVII).
    struct DegreeSpec
    {
        int latticeDegree;      // 1..7
        int accidental;         // relative to major lattice
        ChordQuality quality;
        ChordExtensionSet extensions;
    };

    static const DegreeSpec modernRnBMinor[7] =
    {
        { 1,  0, ChordQuality::minor,          extFlat7 | extNinth },  // i9
        { 2,  0, ChordQuality::halfDiminished, extFlat7 },             // iiø7
        { 3, -1, ChordQuality::major,          extMajor7 | extNinth }, // bIIImaj9
        { 4,  0, ChordQuality::minor,          extFlat7 | extNinth },  // iv9
        { 5,  0, ChordQuality::sus4,           extFlat7 },             // V7sus
        { 6, -1, ChordQuality::major,          extMajor7 | extNinth }, // bVImaj9
        { 7, -1, ChordQuality::dominant,       extNinth }              // bVII9
    };

    const auto& spec = modernRnBMinor[(degree.value - 1) % 7];

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
