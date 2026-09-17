#include "MorphKeyboard.h"
#include "../design_system/MaterialPainters.h"

namespace morph
{

MorphKeyboard::MorphKeyboard()
{
    layout = buildKeyboardLayout (24, 84); // C1..C5: golden range always visible
}

void MorphKeyboard::setState (const MusicalPlaybackState& newState)
{
    state = newState;
    repaint();
}

juce::Rectangle<float> MorphKeyboard::keyBounds (const KeyInfo& k) const
{
    const auto area = getLocalBounds().toFloat();
    const float unitW = area.getWidth() / layout.totalWidthUnits;
    const float x = area.getX() + k.x * unitW;
    const float w = k.width * unitW;
    const float h = k.isBlack ? area.getHeight() * 0.58f : area.getHeight();
    return { x, area.getY(), w, h };
}

int MorphKeyboard::midiAtPoint (juce::Point<float> p) const
{
    // Black keys sit above whites in hit priority.
    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < layout.count; ++i)
        {
            const auto& k = layout.keys[(size_t) i];
            if ((pass == 0) != k.isBlack)
                continue;
            if (keyBounds (k).contains (p))
                return k.midi;
        }
    return -1;
}

void MorphKeyboard::mouseDown (const juce::MouseEvent& e)
{
    const int midi = midiAtPoint (e.position);
    if (midi >= 0)
    {
        mouseHeldPitch = midi;
        if (onNoteEvent)
            onNoteEvent (midi, true, juce::jlimit (0.2f, 1.0f, e.position.y / (float) getHeight() + 0.55f));
        repaint();
    }
}

void MorphKeyboard::mouseUp (const juce::MouseEvent&)
{
    if (mouseHeldPitch >= 0)
    {
        if (onNoteEvent)
            onNoteEvent (mouseHeldPitch, false, 0.0f);
        mouseHeldPitch = -1;
        repaint();
    }
}

void MorphKeyboard::paint (juce::Graphics& g)
{
    const auto lit = computeLitKeys (layout, state);
    const float unitW = getLocalBounds().toFloat().getWidth() / layout.totalWidthUnits;

    // Inset keyboard bed.
    materials::drawRecessedWell (g, getLocalBounds().toFloat().expanded (6.0f, 6.0f), 12.0f);

    // === White keys ===
    for (int i = 0; i < layout.count; ++i)
    {
        const auto& k = layout.keys[(size_t) i];
        if (k.isBlack)
            continue;

        auto b = keyBounds (k).reduced (1.1f, 0.0f);
        const bool held = k.midi == mouseHeldPitch;

        const auto topC = held ? MorphTheme::keyWhite.darker (0.10f) : MorphTheme::keyWhite;
        const auto botC = held ? MorphTheme::keyWhiteShade.darker (0.10f) : MorphTheme::keyWhiteShade;
        juce::ColourGradient grad (topC, b.getX(), b.getY(), botC, b.getX(), b.getBottom(), false);
        g.setGradientFill (grad);

        juce::Path keyPath;
        keyPath.addRoundedRectangle (b.getX(), b.getY(), b.getWidth(), b.getHeight(),
                                     5.0f, 5.0f, false, false, true, true);
        g.fillPath (keyPath);

        // Top inner shadow (under the black-key shelf).
        g.setColour (MorphTheme::keyWhiteShade.darker (0.12f).withAlpha (0.5f));
        g.fillRect (b.getX() + 1.0f, b.getY(), b.getWidth() - 2.0f, 4.0f);

        // Bottom bevel highlight.
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawLine (b.getX() + 4.0f, b.getBottom() - 3.2f, b.getRight() - 4.0f, b.getBottom() - 3.2f, 1.4f);

        // Side hairlines.
        g.setColour (MorphTheme::chassisEdgeShade.withAlpha (0.55f));
        g.drawLine (b.getX(), b.getY(), b.getX(), b.getBottom(), 1.0f);
        g.drawLine (b.getRight(), b.getY(), b.getRight(), b.getBottom(), 1.0f);
    }

    // === Glow spill: halos bleed onto neighbors (reference signature) ===
    for (int i = 0; i < lit.count; ++i)
    {
        const auto* k = layout.find (lit.keys[(size_t) i].midi);
        if (k == nullptr)
            continue;

        const auto b = keyBounds (*k);
        const auto col = MorphTheme::roleColour ((int8_t) lit.keys[(size_t) i].role, true);
        const auto glowCentre = juce::Point<float> (b.getCentreX(), b.getBottom() - b.getHeight() * 0.3f);

        juce::ColourGradient halo (col.withAlpha (0.42f), glowCentre,
                                   col.withAlpha (0.0f),
                                   glowCentre + juce::Point<float> (unitW * 2.2f, 0.0f), true);
        g.setGradientFill (halo);
        g.fillRect (b.expanded (unitW * 2.2f, b.getHeight() * 0.5f));
    }

    // === Lit white keys: colored body ===
    for (int i = 0; i < lit.count; ++i)
    {
        const auto* k = layout.find (lit.keys[(size_t) i].midi);
        if (k == nullptr || k->isBlack)
            continue;

        auto b = keyBounds (*k).reduced (1.1f, 0.0f);
        const auto col = MorphTheme::roleColour ((int8_t) lit.keys[(size_t) i].role, true);

        juce::ColourGradient litGrad (col.brighter (0.35f), b.getX(), b.getY(),
                                      col.darker (0.08f), b.getX(), b.getBottom(), false);
        g.setGradientFill (litGrad);

        juce::Path keyPath;
        keyPath.addRoundedRectangle (b.getX(), b.getY(), b.getWidth(), b.getHeight(),
                                     5.0f, 5.0f, false, false, true, true);
        g.fillPath (keyPath);

        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawLine (b.getX() + 4.0f, b.getBottom() - 3.2f, b.getRight() - 4.0f, b.getBottom() - 3.2f, 1.4f);
    }

    // === Black keys ===
    for (int i = 0; i < layout.count; ++i)
    {
        const auto& k = layout.keys[(size_t) i];
        if (! k.isBlack)
            continue;

        auto b = keyBounds (k).reduced (1.0f, 0.0f);
        const LitKey* lk = lit.find (k.midi);

        juce::Path keyPath;
        keyPath.addRoundedRectangle (b.getX(), b.getY(), b.getWidth(), b.getHeight(),
                                     3.5f, 3.5f, false, false, true, true);

        if (lk != nullptr)
        {
            const auto col = MorphTheme::roleColour ((int8_t) lk->role, true);
            juce::ColourGradient litGrad (col.brighter (0.25f), b.getX(), b.getY(),
                                          col.darker (0.25f), b.getX(), b.getBottom(), false);
            g.setGradientFill (litGrad);
        }
        else
        {
            juce::ColourGradient grad (MorphTheme::keyBlack.brighter (0.42f), b.getX(), b.getY(),
                                       MorphTheme::keyBlack.darker (0.3f), b.getX(), b.getBottom(), false);
            g.setGradientFill (grad);
        }
        g.fillPath (keyPath);

        // Bevel: left light, right shade, bottom catch-light.
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawLine (b.getX() + 1.0f, b.getY() + 2.0f, b.getX() + 1.0f, b.getBottom() - 4.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawLine (b.getRight() - 1.0f, b.getY() + 2.0f, b.getRight() - 1.0f, b.getBottom() - 4.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.22f));
        g.drawLine (b.getX() + 2.0f, b.getBottom() - 2.4f, b.getRight() - 2.0f, b.getBottom() - 2.4f, 1.0f);
    }
}

} // namespace morph
