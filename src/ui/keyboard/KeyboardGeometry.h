#pragma once

#include "../../engine/theory/Pitch.h"
#include "../../engine/voicing/Voicing.h"
#include "../../state/MusicalPlaybackState.h"
#include <array>

namespace morph
{

/** One physical key. Positions are in white-key units (white width = 1.0). */
struct KeyInfo
{
    int midi = -1;
    bool isBlack = false;
    float x = 0.0f;      // left edge, in white-key units
    float width = 1.0f;  // white = 1.0, black = blackKeyWidth
    int whiteIndex = -1; // index among white keys (-1 for black)
};

struct KeyboardLayout
{
    static constexpr int maxKeys = 128;

    std::array<KeyInfo, maxKeys> keys {};
    int count = 0;
    int whiteCount = 0;
    int firstMidi = 24;
    int lastMidi = 84;

    float blackKeyWidth = 0.62f;   // relative to white width
    float totalWidthUnits = 0.0f;  // == whiteCount

    const KeyInfo* find (int midi) const
    {
        for (int i = 0; i < count; ++i)
            if (keys[(size_t) i].midi == midi)
                return &keys[(size_t) i];
        return nullptr;
    }
};

/**
 * Generates keyboard geometry mathematically from MIDI (spec §35).
 * Naturals {0,2,4,5,7,9,11}; accidentals {1,3,6,8,10}; black keys sit on the
 * boundary between their neighboring naturals (never E/F or B/C), forming
 * groups of 2 and 3.
 */
KeyboardLayout buildKeyboardLayout (int firstMidi, int lastMidi);

/** One lit key: exact MIDI pitch + semantic voice-role color. */
struct LitKey
{
    int midi = -1;
    VoiceRole role = VoiceRole::inner;
    float intensity = 1.0f;
};

struct LitKeyList
{
    static constexpr int maxLit = 32;
    std::array<LitKey, maxLit> keys {};
    int count = 0;

    const LitKey* find (int midi) const
    {
        for (int i = 0; i < count; ++i)
            if (keys[(size_t) i].midi == midi)
                return &keys[(size_t) i];
        return nullptr;
    }
};

/**
 * Exact MIDI lighting (spec §36): illuminates exactly the sounding pitches,
 * never pitch classes across octaves. Color comes from VoiceRole (§37).
 */
LitKeyList computeLitKeys (const KeyboardLayout& layout, const MusicalPlaybackState& state);

} // namespace morph
