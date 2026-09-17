#include "MaterialPainters.h"

namespace morph::materials
{

void drawChassisShadow (juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius)
{
    juce::DropShadow shadow (juce::Colour (0x4a3a3020), 34, { 0, 14 });
    shadow.drawForRectangle (g, r.toNearestInt().withY (r.toNearestInt().getY()));
    juce::ignoreUnused (cornerRadius);
}

void drawChassisSurface (juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius)
{
    // Main body: warm bone gradient, slightly lighter at the top.
    juce::ColourGradient body (MorphTheme::chassisTop, r.getX(), r.getY(),
                               MorphTheme::chassisBottom, r.getX(), r.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, cornerRadius);

    // Molded perimeter: bright top-left rim, shaded bottom-right rim.
    g.setColour (MorphTheme::chassisEdgeLight.withAlpha (0.9f));
    g.drawRoundedRectangle (r.reduced (0.8f), cornerRadius, 1.6f);

    g.setColour (MorphTheme::chassisEdgeShade.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (2.4f), cornerRadius - 1.0f, 1.0f);

    // Subtle inner perimeter line (layered molded edge).
    g.setColour (MorphTheme::hairline.withAlpha (0.5f));
    g.drawRoundedRectangle (r.reduced (7.0f), cornerRadius - 6.0f, 0.8f);
}

void drawRecessedWell (juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius)
{
    // Recess: darker than chassis, inner top shadow, faint bottom light.
    g.setColour (MorphTheme::panelInset);
    g.fillRoundedRectangle (r, cornerRadius);

    g.setColour (MorphTheme::chassisEdgeShade.withAlpha (0.45f));
    g.drawRoundedRectangle (r.reduced (0.5f), cornerRadius, 1.0f);

    g.setColour (MorphTheme::chassisEdgeLight.withAlpha (0.55f));
    g.drawLine (r.getX() + cornerRadius, r.getBottom() - 0.5f,
                r.getRight() - cornerRadius, r.getBottom() - 0.5f, 1.0f);
}

void drawRaisedControl (juce::Graphics& g, juce::Rectangle<float> r,
                        float cornerRadius, bool pressed, bool accent)
{
    // Soft contact shadow.
    juce::DropShadow shadow (juce::Colour (0x38302a3a), pressed ? 4 : 9, { 0, pressed ? 1 : 3 });
    juce::Path p;
    p.addRoundedRectangle (r, cornerRadius);
    shadow.drawForPath (g, p);

    // Accent controls stay cream-bodied with an orange outline (reference).
    juce::Colour top = MorphTheme::chassisTop.brighter (0.4f);
    juce::Colour bottom = accent ? MorphTheme::accentOrange.withAlpha (0.22f)
                                 : MorphTheme::chassisBottom;

    if (pressed)
        std::swap (top, bottom);

    juce::ColourGradient body (top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, cornerRadius);

    if (accent && ! pressed)
    {
        g.setColour (MorphTheme::accentOrange.withAlpha (0.9f));
        g.drawRoundedRectangle (r.reduced (1.2f), cornerRadius - 1.0f, 1.4f);
    }
    else
    {
        g.setColour (MorphTheme::chassisEdgeLight.withAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (0.7f), cornerRadius, 1.0f);
    }
}

void drawKnob (juce::Graphics& g, juce::Rectangle<float> r,
               float normalizedValue, juce::Colour capColour, bool highlighted)
{
    const auto centre = r.getCentre();
    const float radius = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;

    // Contact shadow under the knob body.
    juce::DropShadow shadow (juce::Colour (0x453a3026), 8, { 0, 3 });
    juce::Path circle;
    circle.addEllipse (r);
    shadow.drawForPath (g, circle);

    // Outer metal ring.
    juce::ColourGradient ring (MorphTheme::chassisEdgeLight, centre.x, centre.y - radius,
                               MorphTheme::chassisEdgeShade, centre.x, centre.y + radius, false);
    g.setGradientFill (ring);
    g.fillEllipse (r);

    // Molded body.
    const float bodyR = radius * 0.86f;
    juce::ColourGradient body (MorphTheme::knobBody.brighter (0.3f), centre.x, centre.y - bodyR,
                               MorphTheme::knobBody.darker (0.25f), centre.x, centre.y + bodyR, false);
    g.setGradientFill (body);
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (centre));

    // Ceramic colored cap.
    const float capR = radius * 0.62f;
    const auto capRect = juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre);
    juce::ColourGradient cap (capColour.brighter (0.32f), capRect.getX(), capRect.getY(),
                              capColour.darker (0.18f), capRect.getX(), capRect.getBottom(), false);
    g.setGradientFill (cap);
    g.fillEllipse (capRect);

    // Cap top highlight.
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    auto capTop = capRect;
    g.fillEllipse (capTop.removeFromTop (capR * 0.7f).reduced (capR * 0.22f, capR * 0.08f));

    // Pointer: engraved line from cap edge; value 0.5 points straight up.
    const float angle = juce::jmap (normalizedValue, 0.0f, 1.0f,
                                    juce::MathConstants<float>::pi * 0.75f,
                                    juce::MathConstants<float>::pi * 2.25f);
    const auto pointerOuter = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * (radius * 0.94f);
    const auto pointerInner = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * (radius * 0.70f);
    g.setColour (MorphTheme::textPrimary.withAlpha (highlighted ? 1.0f : 0.75f));
    g.drawLine ({ pointerInner, pointerOuter }, highlighted ? 2.6f : 2.0f);

    if (highlighted)
    {
        g.setColour (capColour.withAlpha (0.35f));
        g.drawEllipse (r.expanded (2.5f), 1.6f);
    }
}

void drawRadialSurface (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    // Recessed dark disc.
    juce::ColourGradient deep (MorphTheme::radialDeep.brighter (0.12f), centre.x, centre.y - radius,
                               MorphTheme::radialDeep.darker (0.35f), centre.x, centre.y + radius, false);
    g.setGradientFill (deep);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // Inner shadow at the top of the recess.
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    juce::Path topShadow;
    topShadow.addArc (centre.x - radius + 4, centre.y - radius + 4, radius * 2 - 8, radius * 2 - 8,
                      juce::MathConstants<float>::pi * 1.05f, juce::MathConstants<float>::pi * 1.95f, true);
    g.strokePath (topShadow, juce::PathStrokeType (7.0f));

    // Warm physical rim.
    juce::ColourGradient rim (MorphTheme::chassisEdgeLight, centre.x, centre.y - radius,
                              MorphTheme::chassisEdgeShade, centre.x, centre.y + radius, false);
    g.setGradientFill (rim);
    g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 5.0f);
}

void drawLed (juce::Graphics& g, juce::Point<float> centre, float radius,
              juce::Colour colour, bool lit)
{
    if (lit)
    {
        g.setColour (colour.withAlpha (0.35f));
        g.fillEllipse (centre.x - radius * 2.0f, centre.y - radius * 2.0f, radius * 4.0f, radius * 4.0f);
        g.setColour (colour);
    }
    else
    {
        g.setColour (MorphTheme::chassisEdgeShade.darker (0.35f));
    }
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
}

void drawEngravedLabel (juce::Graphics& g, const juce::String& text,
                        juce::Rectangle<float> area, float size,
                        juce::Colour colour, juce::Justification just)
{
    g.setColour (colour);
    auto font = MorphTheme::labelFont (size);
    font.setExtraKerningFactor (0.14f);
    g.setFont (font);
    g.drawText (text, area, just, true);
}

void drawPuck (juce::Graphics& g, juce::Point<float> centre, float radius,
               juce::Colour body, float glowAmount)
{
    if (glowAmount > 0.01f)
    {
        juce::ColourGradient glow (body.withAlpha (0.5f * glowAmount), centre,
                                   body.withAlpha (0.0f),
                                   centre + juce::Point<float> (radius * 2.4f, 0.0f), true);
        g.setGradientFill (glow);
        g.fillEllipse (centre.x - radius * 2.4f, centre.y - radius * 2.4f,
                       radius * 4.8f, radius * 4.8f);
    }

    // Base shadow.
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (centre.x - radius, centre.y - radius * 0.82f, radius * 2.0f, radius * 2.0f);

    // Sphere body.
    juce::ColourGradient sphere (body.brighter (0.45f),
                                 centre.x - radius * 0.35f, centre.y - radius * 0.45f,
                                 body.darker (0.35f),
                                 centre.x + radius * 0.3f, centre.y + radius * 0.5f, true);
    g.setGradientFill (sphere);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // Top highlight.
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.fillEllipse (centre.x - radius * 0.42f, centre.y - radius * 0.62f,
                   radius * 0.55f, radius * 0.34f);
}

void drawOrb (juce::Graphics& g, juce::Point<float> centre, float radius, float intensity)
{
    const float glow = juce::jlimit (0.0f, 1.0f, intensity);

    // Outer warm glow.
    juce::ColourGradient halo (MorphTheme::orbGlow.withAlpha (0.42f * glow), centre,
                               MorphTheme::orbGlow.withAlpha (0.0f),
                               centre + juce::Point<float> (radius * 3.2f, 0.0f), true);
    g.setGradientFill (halo);
    g.fillEllipse (centre.x - radius * 3.2f, centre.y - radius * 3.2f,
                   radius * 6.4f, radius * 6.4f);

    // Body: dim warm amber at rest, hot cream core while sounding.
    const auto bodyTop = MorphTheme::orbGlow.darker (0.45f).interpolatedWith (MorphTheme::orbCore.brighter (0.2f), glow);
    const auto bodyBottom = MorphTheme::orbGlow.darker (0.55f).interpolatedWith (MorphTheme::orbGlow, glow);
    juce::ColourGradient body (bodyTop, centre.translated (-radius * 0.25f, -radius * 0.3f),
                               bodyBottom, centre.translated (radius, radius), true);
    g.setGradientFill (body);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // Hot core (fades in with activity).
    g.setColour (MorphTheme::orbCore.withAlpha (0.12f + 0.75f * glow));
    g.fillEllipse (centre.x - radius * 0.38f, centre.y - radius * 0.42f,
                   radius * 0.76f, radius * 0.76f);
}

} // namespace morph::materials
