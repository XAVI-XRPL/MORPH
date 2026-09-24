#pragma once

#include "ScheduledNote.h"
#include "../state/MusicalPlaybackState.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace morph
{

/**
 * Realtime-safe MIDI scheduler (spec §59).
 *
 * - Fixed capacity, preallocated; no heap, no locks, no timers.
 * - Sample-accurate event times on an absolute clock; strums remain
 *   correct across block boundaries.
 * - Owns note lifecycle: pending attacks, sounding set, sustain (CC64),
 *   release, retrigger support (common-tone adoption), stuck-note safety.
 *
 * Sounding truth: a pitch is "sounding" exactly between the emission of its
 * note-on and the emission of its note-off. `currentlySoundingNotes` is
 * rebuilt from this set every block.
 */
class MidiScheduler
{
public:
    static constexpr int maxEvents = 256;

    void prepare (double newSampleRate);
    void reset(); // clears everything; emits nothing (use allNotesOff first)

    /** Schedules a performed chord. Returns the group id (> 0). */
    int  scheduleChord (const ScheduledNoteList<maxChordVoices>& notes, int64_t originSample);

    /** Schedules additional events into an existing group (streams refill
        through this so one release kills the whole stream). */
    template <int N>
    void scheduleIntoGroup (int groupId, const ScheduledNoteList<N>& notes, int64_t originSample)
    {
        const int64_t origin = std::max (originSample, clockSamples);
        for (int i = 0; i < notes.count; ++i)
        {
            const auto& n = notes.notes[(size_t) i];

            PendingEvent on;
            on.sampleTime = origin + n.noteOnSampleOffset;
            on.pitch = (uint8_t) n.pitch;
            on.velocity = (uint8_t) juce::jlimit (1, 127, n.velocity);
            on.role = (int8_t) n.role;
            on.groupId = groupId;
            on.isNoteOn = true;
            insertEvent (on);

            if (n.noteOffSampleOffset >= 0)
            {
                PendingEvent off = on;
                off.sampleTime = origin + n.noteOffSampleOffset;
                off.isNoteOn = false;
                insertEvent (off);
            }
        }
    }

    /** Allocates a group id without scheduling events (stream anchor). */
    int  allocateGroup() { return nextGroupId++; }

    /** Sustain-aware release: cancels pending attacks, note-offs sounding
        notes now (or defers them while sustain is held). */
    void releaseGroup (int groupId, int64_t atSample);

    /** Cancels all not-yet-fired events of a group (attacks and releases). */
    void cancelPendingAttacks (int groupId);

    /** Transfers a sounding pitch into another group without re-attack. */
    void adoptSounding (int pitch, int newGroupId);

    /** Sustain-aware release of a single sounding pitch. */
    void releasePitch (int pitch, int64_t atSample);

    /** Group id currently sounding for a pitch, or -1. */
    int soundingGroup (int pitch) const
    {
        return (pitch >= 0 && pitch < 128 && sounding[(size_t) pitch].sounding)
            ? sounding[(size_t) pitch].groupId : -1;
    }

    void setSustain (bool down, int64_t atSample);
    void allNotesOff (int64_t atSample);

    void processBlock (juce::MidiBuffer& out, int numSamples);

    // --- Queries (audio thread) ---
    bool isSounding (int pitch) const;
    int64_t getClock() const { return clockSamples; }
    bool groupHasPendingOrSounding (int groupId) const;
    void rebuildStateBits (MusicalPlaybackState& state) const;

private:
    struct PendingEvent
    {
        int64_t sampleTime = 0;
        uint8_t pitch = 0;
        uint8_t velocity = 0;
        int8_t role = 0;
        int groupId = -1;
        bool isNoteOn = false;
    };

    struct SoundingNote
    {
        bool sounding = false;
        int groupId = -1;
        bool offDeferredBySustain = false;
    };

    void insertEvent (const PendingEvent& e);
    void emitNoteOff (juce::MidiBuffer* out, int pitch, int64_t atSample,
                      int blockStart, int numSamples);
    void fireEvent (const PendingEvent& e, juce::MidiBuffer& out,
                    int64_t blockStart);

    double sampleRate = 44100.0;
    int64_t clockSamples = 0;

    std::array<PendingEvent, maxEvents> events {};
    int eventCount = 0;

    std::array<SoundingNote, 128> sounding {};
    bool sustainDown = false;
    int nextGroupId = 1;
};

} // namespace morph
