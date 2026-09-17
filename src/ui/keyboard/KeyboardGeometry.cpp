#include "KeyboardGeometry.h"
#include <algorithm>

namespace morph
{

KeyboardLayout buildKeyboardLayout (int firstMidi, int lastMidi)
{
    KeyboardLayout layout;
    layout.firstMidi = std::clamp (firstMidi, 0, 127);
    layout.lastMidi = std::clamp (lastMidi, layout.firstMidi, 127);

    int whiteIndex = 0;

    // First pass: naturals.
    for (int midi = layout.firstMidi; midi <= layout.lastMidi; ++midi)
    {
        if (! isNaturalPitchClass (midi))
            continue;

        KeyInfo k;
        k.midi = midi;
        k.isBlack = false;
        k.x = (float) whiteIndex;
        k.width = 1.0f;
        k.whiteIndex = whiteIndex;
        layout.keys[(size_t) layout.count++] = k;
        ++whiteIndex;
    }

    // Second pass: accidentals sit on the boundary before the next natural.
    for (int midi = layout.firstMidi; midi <= layout.lastMidi; ++midi)
    {
        if (! isAccidentalPitchClass (midi))
            continue;

        // Find the natural that follows this accidental.
        int nextWhiteIndex = -1;
        for (int i = 0; i < layout.count; ++i)
        {
            if (layout.keys[(size_t) i].midi > midi)
            {
                nextWhiteIndex = layout.keys[(size_t) i].whiteIndex;
                break;
            }
        }
        if (nextWhiteIndex <= 0)
            continue; // trailing accidental with no following natural in range

        KeyInfo k;
        k.midi = midi;
        k.isBlack = true;
        k.x = (float) nextWhiteIndex - layout.blackKeyWidth * 0.5f;
        k.width = layout.blackKeyWidth;
        k.whiteIndex = -1;
        layout.keys[(size_t) layout.count++] = k;
    }

    layout.whiteCount = whiteIndex;
    layout.totalWidthUnits = (float) whiteIndex;

    // Sort by MIDI pitch (invariant for hit-testing and drawing).
    for (int i = 1; i < layout.count; ++i)
    {
        auto key = layout.keys[(size_t) i];
        int j = i - 1;
        while (j >= 0 && layout.keys[(size_t) j].midi > key.midi)
        {
            layout.keys[(size_t) (j + 1)] = layout.keys[(size_t) j];
            --j;
        }
        layout.keys[(size_t) (j + 1)] = key;
    }

    return layout;
}

LitKeyList computeLitKeys (const KeyboardLayout& layout, const MusicalPlaybackState& state)
{
    LitKeyList lit;

    for (int i = 0; i < layout.count && lit.count < LitKeyList::maxLit; ++i)
    {
        const auto& k = layout.keys[(size_t) i];
        if (! state.currentlySoundingNotes.test ((size_t) k.midi))
            continue;

        LitKey lk;
        lk.midi = k.midi;
        const auto role = state.generatedRoles[(size_t) k.midi];
        lk.role = (role >= 0 && role <= 2) ? (VoiceRole) role : VoiceRole::inner;
        lk.intensity = 1.0f;
        lit.keys[(size_t) lit.count++] = lk;
    }

    return lit;
}

} // namespace morph
