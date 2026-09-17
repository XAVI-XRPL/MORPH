#pragma once

#include "PerformanceTypes.h"
#include "../voicing/Voicing.h"
#include "../../midi/ScheduledNote.h"
#include <cstdint>

namespace morph
{

/**
 * PerformanceEngine: ChordRealization + PerformanceProfile → scheduled notes.
 * Controls attack order, timing, velocity, duration — never harmony.
 * Deterministic: same realization + profile + seed → identical output,
 * so MIDI export reproduces playback exactly (spec §79).
 */
class PerformanceEngine
{
public:
    /** TOGETHER tolerance: all attacks land within this window (spec §91). */
    static constexpr float togetherToleranceMs = 5.0f;

    /**
     * Schedules a realization.
     * @param noteLengthSamples  finite length, or -1 for "until release".
     * @param seed               deterministic humanization seed.
     */
    static ScheduledNoteList<maxChordVoices>
        schedule (const ChordRealization& realization,
                  const PerformanceProfile& profile,
                  double sampleRate,
                  int64_t noteLengthSamples,
                  uint32_t seed);
};

} // namespace morph
