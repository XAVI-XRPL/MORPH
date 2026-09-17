#include "HeaderControls.h"

namespace morph
{

SegmentedPillControl::Zone SegmentedPillControl::zoneAt (float x) const
{
    const float w = (float) getWidth();
    const float segment = juce::jmin (56.0f, w * 0.22f);
    if (x < segment) return Zone::previous;
    if (x > w - segment) return Zone::next;
    return Zone::center;
}

void SegmentedPillControl::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const float radius = r.getHeight() * 0.5f;

    materials::drawRaisedControl (g, r, radius, pressedZone != Zone::none);

    const float w = r.getWidth();
    const float segment = juce::jmin (56.0f, w * 0.22f);

    // Segment dividers.
    g.setColour (MorphTheme::hairline.withAlpha (0.8f));
    g.drawLine (segment, r.getY() + 10.0f, segment, r.getBottom() - 10.0f, 1.0f);
    g.drawLine (w - segment, r.getY() + 10.0f, w - segment, r.getBottom() - 10.0f, 1.0f);

    // Arrows.
    auto drawArrow = [&] (juce::Point<float> centre, bool left, Zone zone)
    {
        const bool active = pressedZone == zone;
        g.setColour (MorphTheme::textPrimary.withAlpha (active ? 1.0f : hoverZone == zone ? 0.9f : 0.6f));
        juce::Path a;
        const float s = 6.5f;
        if (left)
        {
            a.addTriangle (centre.x + s * 0.55f, centre.y - s, centre.x + s * 0.55f, centre.y + s,
                           centre.x - s * 0.55f, centre.y);
        }
        else
        {
            a.addTriangle (centre.x - s * 0.55f, centre.y - s, centre.x - s * 0.55f, centre.y + s,
                           centre.x + s * 0.55f, centre.y);
        }
        g.fillPath (a);
    };

    drawArrow ({ segment * 0.5f, r.getCentreY() }, true, Zone::previous);
    drawArrow ({ w - segment * 0.5f, r.getCentreY() }, false, Zone::next);

    // Label over value.
    auto centreArea = r.withTrimmedLeft (segment).withTrimmedRight (segment);
    auto labelArea = centreArea;
    materials::drawEngravedLabel (g, label, labelArea.removeFromTop (centreArea.getHeight() * 0.42f),
                                  9.5f, MorphTheme::textSecondary);

    g.setColour (MorphTheme::textPrimary);
    g.setFont (MorphTheme::valueFont (17.5f));
    g.drawText (value, centreArea, juce::Justification::centred, true);
}

void SegmentedPillControl::mouseDown (const juce::MouseEvent& e)
{
    pressedZone = zoneAt (e.position.x);
    repaint();
}

void SegmentedPillControl::mouseMove (const juce::MouseEvent& e)
{
    const auto z = zoneAt (e.position.x);
    if (z != hoverZone)
    {
        hoverZone = z;
        repaint();
    }
}

void SegmentedPillControl::mouseUp (const juce::MouseEvent& e)
{
    const auto zone = pressedZone;
    pressedZone = Zone::none;
    repaint();

    if (zoneAt (e.position.x) != zone)
        return;

    if (zone == Zone::previous && onPrevious) onPrevious();
    else if (zone == Zone::next && onNext) onNext();
    else if (zone == Zone::center && onCenter) onCenter();
}

//==============================================================================
void PerformancePillControl::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    materials::drawRaisedControl (g, r, r.getHeight() * 0.5f, pressed);

    const float chevronZone = 34.0f;
    const auto textArea = r.withTrimmedRight (chevronZone);

    g.setColour (MorphTheme::textPrimary);
    auto font = MorphTheme::labelFont (13.5f, true);
    font.setExtraKerningFactor (0.08f);
    g.setFont (font);
    g.drawText (value, textArea, juce::Justification::centred, true);

    // Chevron.
    const auto c = juce::Point<float> (r.getRight() - chevronZone * 0.55f, r.getCentreY());
    g.setColour (MorphTheme::textSecondary);
    juce::Path chev;
    chev.addTriangle (c.x - 5.0f, c.y - 3.0f, c.x + 5.0f, c.y - 3.0f, c.x, c.y + 3.5f);
    g.fillPath (chev);

    if (hover && ! pressed)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    }
}

void PerformancePillControl::mouseUp (const juce::MouseEvent&)
{
    pressed = false;
    repaint();
    if (onClick)
        onClick();
}

//==============================================================================
void MorePillControl::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    materials::drawRaisedControl (g, r, r.getHeight() * 0.5f, pressed);

    auto left = r.withTrimmedRight (r.getWidth() * 0.42f);
    auto right = r.withTrimmedLeft (r.getWidth() * 0.58f);

    auto font = MorphTheme::labelFont (13.0f, true);
    font.setExtraKerningFactor (0.10f);
    g.setFont (font);
    g.setColour (MorphTheme::textPrimary);
    g.drawText ("MORE", left, juce::Justification::centred, true);

    g.setColour (MorphTheme::hairline);
    const float dx = r.getWidth() * 0.58f;
    g.drawLine (dx, r.getY() + 10.0f, dx, r.getBottom() - 10.0f, 1.0f);

    g.setColour (MorphTheme::textSecondary);
    for (int i = 0; i < 3; ++i)
        g.fillEllipse (right.getX() + right.getWidth() * 0.5f - 11.0f + (float) i * 9.0f - 2.0f,
                       r.getCentreY() - 2.0f, 4.0f, 4.0f);
}

void MorePillControl::mouseUp (const juce::MouseEvent&)
{
    pressed = false;
    repaint();
    if (onClick)
        onClick();
}

} // namespace morph
