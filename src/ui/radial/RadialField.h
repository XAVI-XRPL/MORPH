#pragma once

#include "../../state/MusicalPlaybackState.h"
#include "../../engine/progression/Progression.h"
#include "../../engine/theory/KeyContext.h"
#include "../../engine/harmony/StyleProfile.h"
#include "../design_system/MaterialPainters.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace morph
{

/**
 * The Radial Harmonic Field — MORPH's primary visual identity (§22–§28).
 *
 * - Dark recessed circular field with warm physical rim
 * - Concentric guides + crosshair + four cardinal progression pucks
 * - Center Live Orb = current musical truth (luminous while sounding)
 * - Semantic rim arcs = actual sounding voices (bass blue / inner orange /
 *   top yellow), arriving in real attack order during strums
 * - Transition arc between the previous and current progression slot
 */
class RadialField : public juce::Component
{
public:
    RadialField();

    /** Right-click a puck to lock/unlock its slot (§67). */
    std::function<void (int slot)> onSlotLockToggle;

    void setState (const MusicalPlaybackState& newState);
    void setProgression (const Progression& p, const KeyContext& key,
                         const StyleProfile& style);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    int puckAt (juce::Point<float> p) const; // slot index or -1
    void drawGuides (juce::Graphics&, juce::Point<float> centre, float radius);
    void drawVoiceArcs (juce::Graphics&, juce::Point<float> centre, float radius);
    void drawPucks (juce::Graphics&, juce::Point<float> centre, float radius);
    void drawTransitionArc (juce::Graphics&, juce::Point<float> centre, float radius);
    void drawOrbAndLabels (juce::Graphics&, juce::Point<float> centre, float radius);

    /** Pitch → rim angle: low voices lower-left, high voices upper-right. */
    static float pitchToAngle (int midiPitch);

    MusicalPlaybackState state;
    juce::String slotSymbols[4];
    juce::String slotRomans[4];
    juce::Colour slotColours[4];
    bool slotLocked[4] = { false, false, false, false };

    float activity = 0.0f;
    int lastActiveSlot = -1;
    uint32_t slotChangeMs = 0;
    int transitionFrom = -1, transitionTo = -1;
};

} // namespace morph
