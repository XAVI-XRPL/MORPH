#pragma once

#include "MaterialPainters.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace morph
{

/**
 * MORPH LookAndFeel: premium physical digital instrument (§7).
 * Physical knobs with colored ceramic caps, molded buttons,
 * restrained typography.
 */
class MorphLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MorphLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont (juce::Label&) override;

    /** Property keys on components to steer drawing. */
    static inline const char* capColourProperty = "morphCapColour";
    static inline const char* accentProperty = "morphAccent";
    static inline const char* textSizeProperty = "morphTextSize";
};

} // namespace morph
