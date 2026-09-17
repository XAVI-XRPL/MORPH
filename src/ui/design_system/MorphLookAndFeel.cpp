#include "MorphLookAndFeel.h"

namespace morph
{

MorphLookAndFeel::MorphLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, MorphTheme::chassisTop);
    setColour (juce::PopupMenu::textColourId, MorphTheme::textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, MorphTheme::accentOrange.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, MorphTheme::textPrimary);
}

void MorphLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float, float,
                                         juce::Slider& slider)
{
    juce::Colour cap = MorphTheme::knobBody;
    if (auto* v = slider.getProperties().getVarPointer (capColourProperty))
        cap = juce::Colour ((uint32_t) (int64_t) *v);

    materials::drawKnob (g, juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height),
                         sliderPos, cap, slider.isEnabled());
}

void MorphLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                             const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool accent = b.getProperties().getWithDefault (accentProperty, false);
    const float radius = juce::jmin (14.0f, r.getHeight() * 0.30f);

    if (! b.isEnabled())
    {
        g.setColour (MorphTheme::panelInset.withAlpha (0.6f));
        g.fillRoundedRectangle (r, radius);
        g.setColour (MorphTheme::chassisEdgeShade.withAlpha (0.3f));
        g.drawRoundedRectangle (r, radius, 1.0f);
        return;
    }

    materials::drawRaisedControl (g, r, radius, down, accent);

    if (highlighted && ! down)
    {
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (r, radius);
    }
}

void MorphLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b,
                                       bool, bool down)
{
    const float size = (float) b.getProperties().getWithDefault (textSizeProperty, 15.0f);

    auto font = MorphTheme::labelFont (size, true);
    font.setExtraKerningFactor (0.10f);
    g.setFont (font);

    g.setColour (b.isEnabled()
                     ? (b.getProperties().getWithDefault (accentProperty, false)
                            && ! down
                        ? MorphTheme::accentOrange.darker (0.25f)
                        : MorphTheme::textPrimary)
                     : MorphTheme::textSecondary.withAlpha (0.5f));

    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, true);
}

juce::Font MorphLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return MorphTheme::labelFont (juce::jmin (15.0f, buttonHeight * 0.35f), true);
}

juce::Font MorphLookAndFeel::getLabelFont (juce::Label& l)
{
    return MorphTheme::labelFont (l.getFont().getHeight());
}

} // namespace morph
