#pragma once

#include "KeyboardGeometry.h"
#include "../design_system/MorphTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace morph
{

/**
 * The MORPH keyboard: a live truth surface (§34). Displays exactly what the
 * engine outputs (exact pitches, VoiceRole colors) and injects note events
 * when the user clicks keys. M1 geometry/drawing; M4 adds premium materials.
 */
class MorphKeyboard : public juce::Component
{
public:
    std::function<void (int pitch, bool isNoteOn, float velocity)> onNoteEvent;

    MorphKeyboard();

    void setLayout (const KeyboardLayout& newLayout) { layout = newLayout; repaint(); }
    const KeyboardLayout& getLayout() const { return layout; }

    void setState (const MusicalPlaybackState& newState);
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void paint (juce::Graphics&) override;

private:
    int midiAtPoint (juce::Point<float> p) const;
    juce::Rectangle<float> keyBounds (const KeyInfo&) const;

    KeyboardLayout layout;
    MusicalPlaybackState state;
    int mouseHeldPitch = -1;
};

} // namespace morph
