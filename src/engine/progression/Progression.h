#pragma once

#include "../theory/Scale.h"
#include <array>

namespace morph
{

/**
 * A progression slot stores musical INTENT (a scale degree in the current
 * key/style), not fixed MIDI. Playback dynamically realizes exact notes
 * through the same engine pipeline as live performance (spec §31, §63).
 */
struct ProgressionSlot
{
    ScaleDegree degree { 1 };
    bool locked = false;
};

/**
 * Default four-slot progression memory — the first GOLD family (§106):
 *   i9 → bVImaj9 → iv9 → V7sus  (degrees 1, 6, 4, 5 in natural minor)
 */
struct Progression
{
    static constexpr int numSlots = 4;

    std::array<ProgressionSlot, numSlots> slots {
        ProgressionSlot { ScaleDegree { 1 }, false },
        ProgressionSlot { ScaleDegree { 6 }, false },
        ProgressionSlot { ScaleDegree { 4 }, false },
        ProgressionSlot { ScaleDegree { 5 }, false }
    };

    int size = numSlots;
};

} // namespace morph
