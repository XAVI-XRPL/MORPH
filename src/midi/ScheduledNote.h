#pragma once

#include "../engine/voicing/Voicing.h"
#include <array>
#include <cstdint>

namespace morph
{

/** Max voices in one scheduled chord performance. */
inline constexpr int maxChordVoices = 16;

/** One scheduled MIDI note with sample-accurate on/off offsets. */
struct ScheduledNote
{
    int pitch = 60;
    int velocity = 96;
    int64_t noteOnSampleOffset = 0;    // relative to the scheduling origin
    int64_t noteOffSampleOffset = -1;  // -1 = until release
    VoiceRole role = VoiceRole::inner;
};

template <int capacity>
struct ScheduledNoteList
{
    std::array<ScheduledNote, capacity> notes {};
    int count = 0;

    void add (const ScheduledNote& n)
    {
        if (count < capacity)
            notes[(size_t) count++] = n;
    }

    void clear() { count = 0; }
};

} // namespace morph
