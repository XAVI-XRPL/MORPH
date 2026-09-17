#include "../TestHarness.h"
#include "engine/theory/Pitch.h"
#include "engine/theory/Scale.h"
#include "engine/theory/KeyContext.h"
#include "engine/theory/Chord.h"

using namespace morph;

MORPH_TEST (theory, pitchClassMath)
{
    CHECK_EQ (PitchClass { 0 }.value, 0);
    CHECK_EQ (PitchClass { 12 }.value, 0);   // wraps
    CHECK_EQ (PitchClass { -1 }.value, 11);  // wraps negative
    CHECK_EQ (transpose (PitchClass { 0 }, intervals::minorThird).value, 3);
    CHECK_EQ (intervalBetween (PitchClass { 0 }, PitchClass { 10 }).semitones, 10);
    CHECK_EQ (intervalBetween (PitchClass { 10 }, PitchClass { 0 }).semitones, 2);
}

MORPH_TEST (theory, midiPitchConventions)
{
    // C4 = 60 convention (spec §105: C3 = 48, C2 = 36).
    CHECK_EQ (makePitch (PitchClass { 0 }, 4).value, 60);
    CHECK_EQ (makePitch (PitchClass { 0 }, 3).value, 48);
    CHECK_EQ (makePitch (PitchClass { 0 }, 2).value, 36);
    CHECK_EQ (pitchClassOf (MidiPitch { 63 }).value, 3); // Eb4
    CHECK_EQ (octaveOf (MidiPitch { 63 }), 4);
    CHECK_EQ (octaveOf (MidiPitch { 36 }), 2);
}

MORPH_TEST (theory, naturalMinorScale)
{
    const auto minor = ScaleDefinition::naturalMinor();
    CHECK_EQ (minor.size, 7);
    CHECK_EQ (minor.intervalAt (ScaleDegree { 1 }), 0);
    CHECK_EQ (minor.intervalAt (ScaleDegree { 2 }), 2);
    CHECK_EQ (minor.intervalAt (ScaleDegree { 3 }), 3);
    CHECK_EQ (minor.intervalAt (ScaleDegree { 4 }), 5);
    CHECK_EQ (minor.intervalAt (ScaleDegree { 5 }), 7);
    CHECK_EQ (minor.intervalAt (ScaleDegree { 6 }), 8);
    CHECK_EQ (minor.intervalAt (ScaleDegree { 7 }), 10);
}

MORPH_TEST (theory, cMinorKeyContext)
{
    const auto key = KeyContext::cMinor();

    CHECK_EQ (key.pitchClassAtDegree (ScaleDegree { 1 }).value, 0);  // C
    CHECK_EQ (key.pitchClassAtDegree (ScaleDegree { 2 }).value, 2);  // D
    CHECK_EQ (key.pitchClassAtDegree (ScaleDegree { 3 }).value, 3);  // Eb
    CHECK_EQ (key.pitchClassAtDegree (ScaleDegree { 4 }).value, 5);  // F
    CHECK_EQ (key.pitchClassAtDegree (ScaleDegree { 5 }).value, 7);  // G
    CHECK_EQ (key.pitchClassAtDegree (ScaleDegree { 6 }).value, 8);  // Ab
    CHECK_EQ (key.pitchClassAtDegree (ScaleDegree { 7 }).value, 10); // Bb

    CHECK (key.contains (PitchClass { 0 }));
    CHECK (! key.contains (PitchClass { 1 })); // C# is chromatic

    CHECK_EQ (key.degreeOf (PitchClass { 3 })->value, 3);
    CHECK (! key.degreeOf (PitchClass { 6 }).has_value());
}

MORPH_TEST (theory, chordFormulas)
{
    const auto cm9 = ChordFormula::make (ChordQuality::minor, extFlat7 | extNinth);
    CHECK_EQ (cm9.toneCount, 5);
    CHECK_EQ (cm9.semitones[0], 0);
    CHECK_EQ (cm9.semitones[1], 3);
    CHECK_EQ (cm9.semitones[2], 7);
    CHECK_EQ (cm9.semitones[3], 10);
    CHECK_EQ (cm9.semitones[4], 14);

    const auto dom = ChordFormula::make (ChordQuality::dominant, extNone);
    CHECK (hasExtension (dom.extensions, extFlat7)); // dominant implies b7

    const auto halfDim = ChordFormula::make (ChordQuality::halfDiminished, extNone);
    CHECK (hasExtension (halfDim.extensions, extFlat7));
    CHECK_EQ (halfDim.semitones[2], 6); // b5

    const auto sus = ChordFormula::make (ChordQuality::sus4, extFlat7);
    CHECK_EQ (sus.semitones[1], 5); // 4th replaces 3rd
}

MORPH_TEST (theory, romanFunctionDisplay)
{
    CHECK (romanFunctionToString ({ 1, 0, ChordQuality::minor, 0 }) == "i");
    CHECK (romanFunctionToString ({ 6, -1, ChordQuality::major, 0 })
           == juce::String::fromUTF8 ("\xE2\x99\xAD" "VI"));
    CHECK (romanFunctionToString ({ 4, 0, ChordQuality::minor, 0 }) == "iv");
    CHECK (romanFunctionToString ({ 5, 0, ChordQuality::sus4, 0 }) == "V");
    CHECK (romanFunctionToString ({ 2, 0, ChordQuality::halfDiminished, 0 })
           == juce::String::fromUTF8 ("ii\xC3\xB8"));
}

MORPH_TEST (theory, chordSymbolDisplay)
{
    const auto cm9 = ChordSymbol { PitchClass { 0 },
                                   ChordFormula::make (ChordQuality::minor, extFlat7 | extNinth) };
    CHECK (chordSymbolToString (cm9, true) == "Cm9");

    const auto abMaj9 = ChordSymbol { PitchClass { 8 },
                                      ChordFormula::make (ChordQuality::major, extMajor7 | extNinth) };
    CHECK (chordSymbolToString (abMaj9, true) == "Abmaj9");

    const auto fm9 = ChordSymbol { PitchClass { 5 },
                                   ChordFormula::make (ChordQuality::minor, extFlat7 | extNinth) };
    CHECK (chordSymbolToString (fm9, true) == "Fm9");

    const auto g7sus = ChordSymbol { PitchClass { 7 },
                                     ChordFormula::make (ChordQuality::sus4, extFlat7) };
    CHECK (chordSymbolToString (g7sus, true) == "G7sus");
}
