#pragma once

#include "../design_system/MaterialPainters.h"
#include "../design_system/MorphLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace morph
{

/** Physical action button with a minimal drawn glyph (§29). */
class MorphActionButton : public juce::Component
{
public:
    enum class Glyph { none, save, undo, redo, morphWave, play, stop, exploreGrid, midiTarget, moreDots };

    MorphActionButton (juce::String label, Glyph g, bool accent = false);

    std::function<void()> onClick;
    std::function<void()> onDragStart; // MIDI drag-out (§33)
    void setPlaying (bool p); // PLAY toggles glyph play/stop

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { pressed = true; dragged = false; repaint(); }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

    void setDimmed (bool d) { dimmed = d; repaint(); }

private:
    void drawGlyph (juce::Graphics&, juce::Rectangle<float>);

    juce::String label;
    Glyph glyph;
    bool accent;
    bool pressed = false, hover = false, playing = false, dimmed = false;
    bool dragged = false;
};

/**
 * Canonical action row (§29):
 *   SAVE UNDO REDO  |  MORPH  PLAY  |  EXPLORE MIDI MORE
 */
class ActionRow : public juce::Component
{
public:
    ActionRow();

    std::function<void()> onSave, onUndo, onRedo, onMorph, onPlay, onExplore, onMidi, onMore;
    std::function<void()> onMidiDragStart;

    void setPlaying (bool p) { play.setPlaying (p); }
    void setUndoRedoEnabled (bool undo, bool redo);

    void resized() override;

private:
    MorphActionButton save  { "SAVE",    MorphActionButton::Glyph::save };
    MorphActionButton undo  { "UNDO",    MorphActionButton::Glyph::undo };
    MorphActionButton redo  { "REDO",    MorphActionButton::Glyph::redo };
    MorphActionButton morph { "MORPH",   MorphActionButton::Glyph::morphWave, true };
    MorphActionButton play  { "PLAY",    MorphActionButton::Glyph::play };
    MorphActionButton explore { "EXPLORE", MorphActionButton::Glyph::exploreGrid };
    MorphActionButton midi  { "MIDI",    MorphActionButton::Glyph::midiTarget };
    MorphActionButton more  { "MORE",    MorphActionButton::Glyph::moreDots };
};

} // namespace morph
