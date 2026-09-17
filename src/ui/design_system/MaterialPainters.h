#pragma once

#include "MorphTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace morph
{

/**
 * Reusable physical-material drawing primitives (spec §83, §84).
 * Everything is vector-drawn — the interface never uses raster screenshots.
 */
namespace materials
{
    /** Soft outer drop shadow under the chassis (molded, not floating). */
    void drawChassisShadow (juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius);

    /** Main chassis surface: warm bone gradient + layered molded edge. */
    void drawChassisSurface (juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius);

    /** Recessed information well (control banks, keyboard bed). */
    void drawRecessedWell (juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius);

    /** Raised physical pill/button body. pressed = engaged state. */
    void drawRaisedControl (juce::Graphics& g, juce::Rectangle<float> r,
                            float cornerRadius, bool pressed, bool accent = false);

    /** Physical knob: metal ring, molded body, colored ceramic cap, pointer. */
    void drawKnob (juce::Graphics& g, juce::Rectangle<float> r,
                   float normalizedValue, juce::Colour capColour,
                   bool highlighted);

    /** Dark recessed radial surface with warm rim + inner shadow. */
    void drawRadialSurface (juce::Graphics& g, juce::Point<float> centre, float radius);

    /** Small status LED dot (semantic, sparse). */
    void drawLed (juce::Graphics& g, juce::Point<float> centre, float radius,
                  juce::Colour colour, bool lit);

    /** Engraved micro label (small caps, tracked). */
    void drawEngravedLabel (juce::Graphics& g, const juce::String& text,
                            juce::Rectangle<float> area, float size,
                            juce::Colour colour = MorphTheme::textSecondary,
                            juce::Justification just = juce::Justification::centred);

    /** Physical chord puck: small sphere with top highlight + base shadow. */
    void drawPuck (juce::Graphics& g, juce::Point<float> centre, float radius,
                   juce::Colour body, float glowAmount);

    /** Warm luminous orb (the Live Orb). */
    void drawOrb (juce::Graphics& g, juce::Point<float> centre, float radius,
                  float intensity);
}

} // namespace morph
