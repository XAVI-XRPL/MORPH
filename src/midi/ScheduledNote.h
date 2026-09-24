#pragma once

#include "../engine/voicing/Voicing.h"
#include <array>
#include <cstdint>

namespace morph
{

/** Max voices in one scheduled chord performance (up-down strums
    and 6-voice roundtrips can exceed 16). */
inline constexpr int maxChordVoices = 20;

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

    template <int M>
    void assignFrom (const ScheduledNoteList<M>& other)
    {
        clear();
        for (int i = 0; i < other.count; ++i)
            add (other.notes[(size_t) i]);
    }

    void clear() { count = 0; }
};

} // namespace morph
