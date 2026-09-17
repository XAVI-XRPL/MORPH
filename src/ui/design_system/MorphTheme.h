#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace morph
{

/**
 * Canonical MORPH palette — derived from the locked UI reference.
 * Warm bone/cream polymer, dark recessed radial field, semantic voice colors.
 */
struct MorphTheme
{
    // Chassis / enclosure
    static inline const juce::Colour windowBackground { 0xffeceae5 };
    static inline const juce::Colour chassisTop       { 0xfff2efe9 };
    static inline const juce::Colour chassisBottom    { 0xffddd9d1 };
    static inline const juce::Colour chassisEdgeLight { 0xffffffff };
    static inline const juce::Colour chassisEdgeShade { 0xffb8b2a8 };
    static inline const juce::Colour panelInset       { 0xffe2ded6 };
    static inline const juce::Colour hairline         { 0xffc6c1b7 };

    // Radial field
    static inline const juce::Colour radialDeep       { 0xff211e1a };
    static inline const juce::Colour radialRing       { 0xff2e2a25 };
    static inline const juce::Colour radialGuide      { 0xff4a443c };
    static inline const juce::Colour orbCore          { 0xffffd9a0 };
    static inline const juce::Colour orbGlow          { 0xffff9e3d };

    // Semantic voice colors (locked: §27, §109)
    static inline const juce::Colour bassBlue         { 0xff5e86c4 };
    static inline const juce::Colour bassBlueGlow     { 0xff6fa3e8 };
    static inline const juce::Colour innerOrange      { 0xffe8873f };
    static inline const juce::Colour innerOrangeGlow  { 0xfff0a05c };
    static inline const juce::Colour topYellow        { 0xfff2c94c };
    static inline const juce::Colour topYellowGlow    { 0xffffd966 };
    static inline const juce::Colour tensionViolet    { 0xff9b7fd4 };
    static inline const juce::Colour inputGreen       { 0xff7fb069 };

    // Text
    static inline const juce::Colour textPrimary      { 0xff3b3733 };
    static inline const juce::Colour textSecondary    { 0xff8a847c };
    static inline const juce::Colour textOnDark       { 0xfff0ece4 };

    // Controls
    static inline const juce::Colour knobBody         { 0xffefece5 };
    static inline const juce::Colour keyWhite         { 0xfff7f5f0 };
    static inline const juce::Colour keyWhiteShade    { 0xffd9d4cb };
    static inline const juce::Colour keyBlack         { 0xff2a2724 };
    static inline const juce::Colour accentOrange     { 0xffe8783c };
    static inline const juce::Colour playGreen        { 0xff4da35e };

    static juce::Colour roleColour (int8_t role, bool glow = false);
    static juce::Font labelFont (float size, bool bold = false);
    static juce::Font valueFont (float size);
};

} // namespace morph
