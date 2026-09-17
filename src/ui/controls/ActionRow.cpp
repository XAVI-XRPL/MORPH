#include "ActionRow.h"

namespace morph
{

MorphActionButton::MorphActionButton (juce::String l, Glyph g, bool acc)
    : label (std::move (l)), glyph (g), accent (acc)
{
}

void MorphActionButton::setPlaying (bool p)
{
    playing = p;
    repaint();
}

void MorphActionButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);

    if (dimmed)
    {
        g.setColour (MorphTheme::panelInset.withAlpha (0.55f));
        g.fillRoundedRectangle (r, r.getHeight() * 0.28f);
        g.setColour (MorphTheme::chassisEdgeShade.withAlpha (0.3f));
        g.drawRoundedRectangle (r, r.getHeight() * 0.28f, 1.0f);
    }
    else
    {
        materials::drawRaisedControl (g, r, r.getHeight() * 0.28f, pressed, accent);

        if (hover && ! pressed)
        {
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillRoundedRectangle (r, r.getHeight() * 0.28f);
        }
    }

    auto content = r.reduced (10.0f, 0.0f);
    auto glyphArea = content.removeFromLeft (30.0f).reduced (3.0f);

    const auto textCol = dimmed ? MorphTheme::textSecondary.withAlpha (0.45f)
                       : accent   ? MorphTheme::accentOrange.darker (0.22f)
                                  : MorphTheme::textPrimary;

    drawGlyph (g, glyphArea);

    auto font = MorphTheme::labelFont (12.0f, true);
    font.setExtraKerningFactor (0.10f);
    g.setFont (font);
    g.setColour (textCol);
    g.drawText (playing && glyph == Glyph::play ? "STOP" : label, content,
                juce::Justification::centred, true);

    // Status dot on the two signature actions.
    if (accent || glyph == Glyph::play)
    {
        const auto dotCol = accent ? MorphTheme::accentOrange : MorphTheme::playGreen;
        materials::drawLed (g, { r.getRight() - 12.0f, r.getCentreY() }, 3.4f, dotCol,
                            playing || accent);
    }
}

void MorphActionButton::drawGlyph (juce::Graphics& g, juce::Rectangle<float> a)
{
    const auto c = a.getCentre();
    const float s = a.getHeight() * 0.5f;
    const auto col = (accent ? MorphTheme::accentOrange : MorphTheme::textPrimary)
                         .withAlpha (dimmed ? 0.4f : 0.9f);
    g.setColour (col);

    switch (glyph)
    {
        case Glyph::save:
        {
            g.drawRoundedRectangle (c.x - s * 0.7f, c.y - s * 0.7f, s * 1.4f, s * 1.4f, 2.0f, 1.6f);
            g.fillRect (c.x - s * 0.28f, c.y - s * 0.28f, s * 0.56f, s * 0.56f);
            break;
        }
        case Glyph::undo:
        case Glyph::redo:
        {
            juce::Path arc;
            arc.addArc (c.x - s * 0.62f, c.y - s * 0.62f, s * 1.24f, s * 1.24f,
                        glyph == Glyph::undo ? 0.4f : -juce::MathConstants<float>::pi + 0.6f,
                        glyph == Glyph::undo ? juce::MathConstants<float>::pi + 1.4f : -0.4f, true);
            g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            break;
        }
        case Glyph::morphWave:
        {
            juce::Path w;
            w.startNewSubPath (c.x - s * 0.7f, c.y);
            w.cubicTo (c.x - s * 0.35f, c.y - s * 0.9f, c.x - s * 0.1f, c.y + s * 0.9f,
                       c.x + s * 0.2f, c.y);
            w.cubicTo (c.x + s * 0.45f, c.y - s * 0.7f, c.x + s * 0.55f, c.y - s * 0.3f,
                       c.x + s * 0.8f, c.y - s * 0.5f);
            g.strokePath (w, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
            break;
        }
        case Glyph::play:
        case Glyph::stop:
        {
            if (playing)
            {
                g.fillRect (c.x - s * 0.5f, c.y - s * 0.5f, s, s);
            }
            else
            {
                g.setColour (MorphTheme::playGreen);
                juce::Path tri;
                tri.addTriangle (c.x - s * 0.45f, c.y - s * 0.6f, c.x - s * 0.45f, c.y + s * 0.6f,
                                 c.x + s * 0.62f, c.y);
                g.fillPath (tri);
            }
            break;
        }
        case Glyph::exploreGrid:
        {
            for (int i = 0; i < 4; ++i)
            {
                const float gx = c.x - s * 0.62f + (float) (i % 2) * s * 0.75f;
                const float gy = c.y - s * 0.62f + (float) (i / 2) * s * 0.75f;
                g.fillRoundedRectangle (gx, gy, s * 0.5f, s * 0.5f, 1.5f);
            }
            break;
        }
        case Glyph::midiTarget:
        {
            g.drawEllipse (c.x - s * 0.7f, c.y - s * 0.7f, s * 1.4f, s * 1.4f, 1.6f);
            g.fillEllipse (c.x - s * 0.25f, c.y - s * 0.25f, s * 0.5f, s * 0.5f);
            break;
        }
        case Glyph::moreDots:
        {
            for (int i = 0; i < 3; ++i)
                g.fillEllipse (c.x - s * 0.7f + (float) i * s * 0.7f, c.y - 2.0f, 4.0f, 4.0f);
            break;
        }
        case Glyph::none:
            break;
    }
}

void MorphActionButton::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragged && onDragStart && e.getDistanceFromDragStart() > 8)
    {
        dragged = true;
        onDragStart();
    }
}

void MorphActionButton::mouseUp (const juce::MouseEvent&)
{
    pressed = false;
    repaint();
    if (! dimmed && onClick)
        onClick();
}

//==============================================================================
ActionRow::ActionRow()
{
    save.onClick = [this] { if (onSave) onSave(); };
    undo.onClick = [this] { if (onUndo) onUndo(); };
    redo.onClick = [this] { if (onRedo) onRedo(); };
    morph.onClick = [this] { if (onMorph) onMorph(); };
    play.onClick = [this] { if (onPlay) onPlay(); };
    explore.onClick = [this] { if (onExplore) onExplore(); };
    midi.onClick = [this] { if (onMidi) onMidi(); };
    midi.onDragStart = [this] { if (onMidiDragStart) onMidiDragStart(); };
    more.onClick = [this] { if (onMore) onMore(); };

    for (auto* b : { &save, &undo, &redo, &morph, &play, &explore, &midi, &more })
        addAndMakeVisible (*b);

    undo.setDimmed (false);
    redo.setDimmed (true); // nothing to redo at rest — matches the reference
}

void ActionRow::setUndoRedoEnabled (bool u, bool r)
{
    undo.setDimmed (! u);
    redo.setDimmed (! r);
}

void ActionRow::resized()
{
    auto area = getLocalBounds();
    const int h = area.getHeight();
    const int small = 120, big = 150, gap = 14;

    auto left = area.removeFromLeft (small * 3 + gap * 2);
    save.setBounds (left.removeFromLeft (small));
    left.removeFromLeft (gap);
    undo.setBounds (left.removeFromLeft (small));
    left.removeFromLeft (gap);
    redo.setBounds (left.removeFromLeft (small));

    auto right = area.removeFromRight (small * 3 + gap * 2);
    more.setBounds (right.removeFromRight (small));
    right.removeFromRight (gap);
    midi.setBounds (right.removeFromRight (small));
    right.removeFromRight (gap);
    explore.setBounds (right.removeFromRight (small));

    // Center: MORPH + PLAY with slightly more emphasis (§29).
    auto centre = area.reduced (0, 0).withSizeKeepingCentre (big * 2 + gap + 30, h);
    auto centreArea = centre;
    morph.setBounds (centreArea.removeFromLeft (big + 15));
    centreArea.removeFromLeft (gap);
    play.setBounds (centreArea.removeFromLeft (big + 15));

    // Hairline separators flanking the signature actions.
    juce::ignoreUnused (h);
}

} // namespace morph
