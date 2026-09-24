#pragma once

#include "PerformanceTypes.h"
#include "../voicing/Voicing.h"
#include "../../midi/ScheduledNote.h"
#include <cstdint>

namespace morph
{

/**
 * PatternEngine (M8, §102): generates PULSE / PATTERN / ARP performance
 * events for an absolute-sample window. Pure function of (realization,
 * profile, tempo, absolute musical position, seed) — window splits never
 * change the output, so refill == continuation (export == playback).
 *
 * Called on the audio thread in bounded per-block steps (fixed-capacity
 * output, no allocation).
 */
struct StreamContext
{
    ChordRealization realization;
    PerformanceProfile profile;
    double sampleRate = 44100.0;
    double bpm = 80.0;
    uint32_t seed = 1;
    int64_t streamStartSample = 0; // absolute sample of grid point 0
};

class PatternEngine
{
public:
    /** Fills `out` with events whose note-ons fall in [windowStart, windowEnd)
        (absolute samples). Event offsets are relative to streamStartSample. */
    static void generateWindow (const StreamContext& ctx,
                                int64_t windowStart, int64_t windowEnd,
                                ScheduledNoteList<64>& out);

    /** Grid subdivision length in samples for a rate at a tempo. */
    static int64_t rateToSamples (StreamRate rate, double sampleRate, double bpm);

private:
    static void generatePulse (const StreamContext&, int64_t w0, int64_t w1, ScheduledNoteList<64>&);
    static void generateArp (const StreamContext&, int64_t w0, int64_t w1, ScheduledNoteList<64>&);
    static void generatePattern (const StreamContext&, int64_t w0, int64_t w1, ScheduledNoteList<64>&);
};

} // namespace morph
