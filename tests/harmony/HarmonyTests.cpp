#include "../TestHarness.h"
#include "engine/harmony/TriggerInterpreter.h"
#include "engine/harmony/HarmonyEngine.h"

using namespace morph;

MORPH_TEST (harmony, scaleDegreeMappingCMinor)
{
    const auto key = KeyContext::cMinor();
    TriggerInterpreter interpreter;

    // Spec §45: C → i, D → ii°, Eb → bIII, F → iv, G → v/V, Ab → bVI, Bb → bVII
    CHECK_EQ (interpreter.interpret (MidiPitch { 48 }, key).degree.value, 1); // C3
    CHECK_EQ (interpreter.interpret (MidiPitch { 50 }, key).degree.value, 2); // D
    CHECK_EQ (interpreter.interpret (MidiPitch { 51 }, key).degree.value, 3); // Eb
    CHECK_EQ (interpreter.interpret (MidiPitch { 53 }, key).degree.value, 4); // F
    CHECK_EQ (interpreter.interpret (MidiPitch { 55 }, key).degree.value, 5); // G
    CHECK_EQ (interpreter.interpret (MidiPitch { 56 }, key).degree.value, 6); // Ab
    CHECK_EQ (interpreter.interpret (MidiPitch { 58 }, key).degree.value, 7); // Bb
}

MORPH_TEST (harmony, chromaticInputMapsToValidFunction)
{
    const auto key = KeyContext::cMinor();
    TriggerInterpreter interpreter;

    // C# (chromatic in C minor) must not produce an arbitrary chromatic chord:
    // STYLE_COLOR maps it to the nearest valid degree (downward on ties).
    const auto result = interpreter.interpret (MidiPitch { 49 }, key); // C#3
    CHECK (result.wasChromatic);
    CHECK_EQ (result.degree.value, 1);
}

MORPH_TEST (harmony, modernRnBVocabulary)
{
    const auto key = KeyContext::cMinor();
    const auto style = StyleProfile::modernRnB();
    HarmonyEngine harmony;

    const auto i = harmony.chordForDegree (key, ScaleDegree { 1 }, style, 0.5f);
    CHECK_EQ (i.chord.root.value, 0);
    CHECK_EQ (i.chord.formula.quality, ChordQuality::minor);
    CHECK (hasExtension (i.chord.formula.extensions, extNinth));
    CHECK (romanFunctionToString (i.roman) == "i");

    const auto bVI = harmony.chordForDegree (key, ScaleDegree { 6 }, style, 0.5f);
    CHECK_EQ (bVI.chord.root.value, 8); // Ab
    CHECK (hasExtension (bVI.chord.formula.extensions, extMajor7));
    CHECK_EQ (bVI.roman.accidental, -1);

    const auto iv = harmony.chordForDegree (key, ScaleDegree { 4 }, style, 0.5f);
    CHECK_EQ (iv.chord.root.value, 5); // F
    CHECK_EQ (iv.chord.formula.quality, ChordQuality::minor);

    const auto V = harmony.chordForDegree (key, ScaleDegree { 5 }, style, 0.5f);
    CHECK_EQ (V.chord.root.value, 7); // G
    CHECK_EQ (V.chord.formula.quality, ChordQuality::sus4);
}

MORPH_TEST (harmony, styleVocabulariesDiffer)
{
    // M6: HarmonyEngine reads the style's degree map — the same degree
    // produces the style's chord identity.
    const auto key = KeyContext::cMinor();
    HarmonyEngine harmony;

    const auto rnbV = harmony.chordForDegree (key, ScaleDegree { 5 },
                                              StyleProfile::get (StyleId::modernRnB), 0.5f);
    CHECK (rnbV.chord.formula.quality == ChordQuality::sus4);
    CHECK (chordSymbolToString (rnbV.chord, true) == "G7sus");

    const auto darkV = harmony.chordForDegree (key, ScaleDegree { 5 },
                                               StyleProfile::get (StyleId::darkRnB), 0.5f);
    CHECK (darkV.chord.formula.quality == ChordQuality::minor); // minor v: darker
    CHECK (chordSymbolToString (darkV.chord, true) == "Gm7");

    const auto trapI = harmony.chordForDegree (key, ScaleDegree { 1 },
                                               StyleProfile::get (StyleId::trap), 0.5f);
    CHECK (! hasExtension (trapI.chord.formula.extensions, extNinth)); // stark Cm7
    CHECK (chordSymbolToString (trapI.chord, true) == "Cm7");

    const auto neoSoulIV = harmony.chordForDegree (key, ScaleDegree { 4 },
                                                   StyleProfile::get (StyleId::neoSoul), 0.5f);
    CHECK (hasExtension (neoSoulIV.chord.formula.extensions, extEleventh)); // Fm11
}

MORPH_TEST (harmony, colorKnobScalesBrightness)
{
    const auto key = KeyContext::cMinor();
    const auto style = StyleProfile::modernRnB();
    HarmonyEngine harmony;

    const auto dark = harmony.chordForDegree (key, ScaleDegree { 1 }, style, 0.1f);
    CHECK (! hasExtension (dark.chord.formula.extensions, extNinth));

    const auto canonical = harmony.chordForDegree (key, ScaleDegree { 1 }, style, 0.5f);
    CHECK (hasExtension (canonical.chord.formula.extensions, extNinth));
    CHECK (! hasExtension (canonical.chord.formula.extensions, extEleventh));

    const auto bright = harmony.chordForDegree (key, ScaleDegree { 1 }, style, 0.9f);
    CHECK (hasExtension (bright.chord.formula.extensions, extNinth));
    CHECK (hasExtension (bright.chord.formula.extensions, extEleventh));
}
