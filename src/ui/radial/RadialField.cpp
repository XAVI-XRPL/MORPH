#include "RadialField.h"
#include "../../engine/harmony/HarmonyEngine.h"

namespace morph
{

RadialField::RadialField()
{
    // Per-slot accents matching the reference (I blue, IV yellow, bVII/V orange).
    slotColours[0] = MorphTheme::bassBlueGlow;
    slotColours[1] = MorphTheme::topYellow;
    slotColours[2] = MorphTheme::innerOrange;
    slotColours[3] = MorphTheme::accentOrange;
    for (int i = 0; i < 4; ++i)
    {
        slotSymbols[i] = "--";
        slotRomans[i] = "--";
    }
}

void RadialField::setProgression (const Progression& p, const KeyContext& key,
                                  const StyleProfile& style)
{
    HarmonyEngine harmony;
    for (int i = 0; i < 4 && i < p.size; ++i)
    {
        const auto candidate = harmony.chordForDegree (key, p.slots[(size_t) i].degree, style, 0.5f);
        slotSymbols[i] = chordSymbolToString (candidate.chord, key.prefersFlats());
        slotRomans[i] = romanFunctionToString (candidate.roman);
    }
    repaint();
}

void RadialField::setState (const MusicalPlaybackState& newState)
{
    if (state.activeProgressionSlot != newState.activeProgressionSlot)
    {
        transitionFrom = state.activeProgressionSlot;
        transitionTo = newState.activeProgressionSlot;
        slotChangeMs = juce::Time::getMillisecondCounter();
    }
    state = newState;
    repaint();
}

float RadialField::pitchToAngle (int midiPitch)
{
    // 36 (low) → 225° (lower-left), 72+ (high) → -45°/315° (upper-right).
    const float norm = juce::jlimit (0.0f, 1.0f, ((float) midiPitch - 30.0f) / 54.0f);
    return juce::jmap (norm, juce::MathConstants<float>::pi * 1.25f,
                            juce::MathConstants<float>::pi * 1.75f);
}

void RadialField::drawGuides (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    // Crosshair.
    g.setColour (MorphTheme::radialGuide.withAlpha (0.55f));
    g.drawLine (centre.x - radius * 0.88f, centre.y, centre.x + radius * 0.88f, centre.y, 0.8f);
    g.drawLine (centre.x, centre.y - radius * 0.88f, centre.x, centre.y + radius * 0.88f, 0.8f);

    // Crosshair tick at center.
    g.setColour (MorphTheme::radialGuide.brighter (0.3f));
    g.fillEllipse (centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);

    // Concentric dotted orbit lines.
    for (float r : { 0.34f, 0.55f, 0.76f })
    {
        const float orbitR = radius * r;
        const int dots = 48;
        g.setColour (MorphTheme::radialGuide.withAlpha (0.65f));
        for (int i = 0; i < dots; ++i)
        {
            const float a = (float) i / (float) dots * juce::MathConstants<float>::twoPi;
            const auto p = centre + juce::Point<float> (std::cos (a), std::sin (a)) * orbitR;
            g.fillEllipse (p.x - 0.8f, p.y - 0.8f, 1.6f, 1.6f);
        }
    }

    // Rim label: RADIAL HARMONIC FIELD around the top inner edge.
    materials::drawEngravedLabel (g, "RADIAL HARMONIC FIELD",
                                  { centre.x - radius, centre.y - radius * 0.985f,
                                    radius * 2.0f, 13.0f },
                                  8.5f, MorphTheme::textOnDark.withAlpha (0.5f));
}

void RadialField::drawVoiceArcs (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    // One rim arc per sounding voice, colored by role, arriving in real
    // attack order (§28, §39). Sparse and meaningful — no decoration.
    const float arcRadius = radius * 0.90f;
    const float arcSpan = juce::MathConstants<float>::pi / 7.0f;

    for (int midi = 0; midi < 128; ++midi)
    {
        if (! state.currentlySoundingNotes.test ((size_t) midi))
            continue;

        const auto role = state.generatedRoles[(size_t) midi];
        const auto col = MorphTheme::roleColour (role, true);
        const float a = pitchToAngle (midi);

        juce::Path arc;
        arc.addArc (centre.x - arcRadius, centre.y - arcRadius,
                    arcRadius * 2.0f, arcRadius * 2.0f,
                    a - arcSpan, a + arcSpan, true);

        g.setColour (col.withAlpha (0.85f));
        g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }
}

void RadialField::drawPucks (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    // Cardinal positions (§25): slot 1 left, slot 2 top, slot 3 right, slot 4 bottom.
    const juce::Point<float> pos[4] =
    {
        { centre.x - radius * 0.64f, centre.y },
        { centre.x, centre.y - radius * 0.64f },
        { centre.x + radius * 0.64f, centre.y },
        { centre.x, centre.y + radius * 0.64f }
    };

    const float pr = radius * 0.10f;

    for (int i = 0; i < 4; ++i)
    {
        const bool isActive = state.activeProgressionSlot == i;
        const bool isMatch = state.hasActiveChord
            && chordSymbolToString (state.activeChord, true) == slotSymbols[i];
        const float glow = isActive ? 1.0f : isMatch ? 0.85f : 0.0f;

        materials::drawPuck (g, pos[i], pr, slotColours[i], glow);

        materials::drawEngravedLabel (g, slotRomans[i],
                                      { pos[i].x - pr * 2.2f, pos[i].y - pr * 2.35f, pr * 4.4f, 15.0f },
                                      12.5f, MorphTheme::textOnDark.withAlpha (0.92f));
        materials::drawEngravedLabel (g, slotSymbols[i],
                                      { pos[i].x - pr * 2.2f, pos[i].y + pr * 1.45f, pr * 4.4f, 14.0f },
                                      11.0f, MorphTheme::textOnDark.withAlpha (0.68f));
    }
}

void RadialField::drawTransitionArc (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    if (transitionFrom < 0 || transitionTo < 0)
        return;

    const auto elapsed = (int) (juce::Time::getMillisecondCounter() - slotChangeMs);
    if (elapsed > 700)
    {
        transitionFrom = transitionTo = -1;
        return;
    }

    const float fade = 1.0f - (float) elapsed / 700.0f;

    const juce::Point<float> pos[4] =
    {
        { centre.x - radius * 0.64f, centre.y },
        { centre.x, centre.y - radius * 0.64f },
        { centre.x + radius * 0.64f, centre.y },
        { centre.x, centre.y + radius * 0.64f }
    };

    const auto from = pos[transitionFrom];
    const auto to = pos[transitionTo];

    // Controlled arc bowing through the field between the two slots.
    juce::Path p;
    p.startNewSubPath (from);
    const auto mid = (from + to) * 0.5f;
    const auto control = centre + (mid - centre) * 0.35f;
    p.quadraticTo (control, to);

    g.setColour (MorphTheme::orbGlow.withAlpha (0.55f * fade));
    g.strokePath (p, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}

void RadialField::drawOrbAndLabels (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    const bool active = state.currentlySoundingNotes.any() || state.pendingScheduledNotes.any();
    activity += ((active ? 1.0f : 0.3f) - activity) * 0.18f; // ~90–220 ms ease (§86)

    const float orbR = radius * 0.13f * (1.0f + 0.14f * activity);
    materials::drawOrb (g, centre, orbR, activity);

    if (state.hasActiveChord)
    {
        g.setColour (MorphTheme::textOnDark.withAlpha (0.95f));
        g.setFont (MorphTheme::valueFont (orbR * 0.72f));
        g.drawText (chordSymbolToString (state.activeChord, true),
                    juce::Rectangle<float> (centre.x - orbR * 2.4f, centre.y + orbR * 1.3f,
                                            orbR * 4.8f, orbR * 0.75f),
                    juce::Justification::centred);

        materials::drawEngravedLabel (g, romanFunctionToString (state.activeRomanFunction),
                                      { centre.x - orbR * 2.4f, centre.y - orbR * 2.05f,
                                        orbR * 4.8f, orbR * 0.7f },
                                      13.0f, MorphTheme::textOnDark.withAlpha (0.75f));
    }
}

void RadialField::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const float side = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float radius = side * 0.47f;

    materials::drawRadialSurface (g, centre, radius);

    drawGuides (g, centre, radius);
    drawTransitionArc (g, centre, radius);
    drawVoiceArcs (g, centre, radius);
    drawPucks (g, centre, radius);
    drawOrbAndLabels (g, centre, radius);
}

} // namespace morph
