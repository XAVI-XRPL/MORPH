#include "Chord.h"
#include <juce_core/juce_core.h>

namespace morph
{

ChordFormula ChordFormula::make (ChordQuality q, ChordExtensionSet ext)
{
    ChordFormula f;
    f.quality = q;
    f.extensions = ext;

    auto push = [&f] (int semis)
    {
        if (f.toneCount < maxTones)
            f.semitones[(size_t) f.toneCount++] = semis;
    };

    // Triad
    switch (q)
    {
        case ChordQuality::major:
        case ChordQuality::dominant:    push (0); push (4); push (7); break;
        case ChordQuality::minor:       push (0); push (3); push (7); break;
        case ChordQuality::diminished:
        case ChordQuality::halfDiminished: push (0); push (3); push (6); break;
        case ChordQuality::augmented:   push (0); push (4); push (8); break;
        case ChordQuality::sus2:        push (0); push (2); push (7); break;
        case ChordQuality::sus4:        push (0); push (5); push (7); break;
    }

    // Seventh: implied by quality where applicable
    if (q == ChordQuality::dominant || q == ChordQuality::halfDiminished)
        ext |= extFlat7;

    f.extensions = ext; // record the effective set (incl. implied sevenths)

    if (hasExtension (ext, extFlat7))  push (10);
    if (hasExtension (ext, extMajor7)) push (11);

    // Upper extensions
    if (hasExtension (ext, extNinth))    push (14);
    if (hasExtension (ext, extFlat9))    push (13);
    if (hasExtension (ext, extSharp9))   push (15);
    if (hasExtension (ext, extEleventh)) push (17);
    if (hasExtension (ext, extSharp11))  push (18);
    if (hasExtension (ext, extFlat13))   push (20);
    if (hasExtension (ext, ext13th))     push (21);

    return f;
}

namespace
{
    juce::String romanNumeral (int degree)
    {
        static const char* numerals[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
        return numerals[juce::jlimit (1, 7, degree) - 1];
    }

    bool isLowercaseQuality (ChordQuality q)
    {
        return q == ChordQuality::minor
            || q == ChordQuality::diminished
            || q == ChordQuality::halfDiminished;
    }

    int highestExtensionNumber (ChordExtensionSet ext)
    {
        if (ext & (ext13th | extFlat13))   return 13;
        if (ext & (extEleventh | extSharp11)) return 11;
        if (ext & (extNinth | extFlat9 | extSharp9)) return 9;
        if (ext & (extFlat7 | extMajor7))  return 7;
        return 0;
    }
}

juce::String romanFunctionToString (RomanFunction fn)
{
    juce::String s;
    if (fn.accidental < 0) s << juce::String::fromUTF8 ("\xE2\x99\xAD"); // flat sign
    if (fn.accidental > 0) s << juce::String::fromUTF8 ("\xE2\x99\xAF"); // sharp sign

    auto numeral = romanNumeral (fn.degree);
    s << (isLowercaseQuality (fn.quality) ? numeral.toLowerCase() : numeral);

    if (fn.quality == ChordQuality::halfDiminished)
        s << juce::String::fromUTF8 ("\xC3\xB8"); // o-slash
    else if (fn.quality == ChordQuality::diminished)
        s << juce::String::fromUTF8 ("\xC2\xB0"); // degree sign

    return s;
}

juce::String chordSymbolToString (ChordSymbol chord, bool useFlats)
{
    juce::String s (pitchClassName (chord.root, useFlats));

    const auto q = chord.formula.quality;
    const auto ext = chord.formula.extensions;
    const int top = highestExtensionNumber (ext);

    switch (q)
    {
        case ChordQuality::minor:
            s << "m";
            if (top > 0) s << juce::String (top);
            break;

        case ChordQuality::major:
            if (ext & extMajor7)
                s << (top > 7 ? "maj" + juce::String (top) : "maj7");
            else if (top > 0)
                s << juce::String (top == 7 ? "7" : juce::String (top));
            break;

        case ChordQuality::dominant:
            s << juce::String (top >= 7 ? top : 7);
            break;

        case ChordQuality::sus4:
            if (ext & extFlat7) s << juce::String (top >= 7 ? top : 7);
            s << "sus";
            break;

        case ChordQuality::sus2:
            s << "sus2";
            if (top > 0) s << juce::String (top);
            break;

        case ChordQuality::halfDiminished:
            s << juce::String::fromUTF8 ("\xC3\xB8") << juce::String (top >= 7 ? top : 7);
            break;

        case ChordQuality::diminished:
            s << juce::String::fromUTF8 ("\xC2\xB0");
            if (top > 0) s << juce::String (top);
            break;

        case ChordQuality::augmented:
            s << "aug";
            if (top > 0) s << juce::String (top);
            break;
    }

    return s;
}

} // namespace morph
