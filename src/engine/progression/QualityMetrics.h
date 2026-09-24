#pragma once

#include "../harmony/StyleProfile.h"
#include "../voicing/Voicing.h"
#include <array>

namespace morph
{

/**
 * Internal progression quality metrics (§71) — never exposed raw to users.
 * All components normalized ~0..1; total is the weighted blend used by the
 * generator and the bank validator.
 */
struct QualityScores
{
    float functionalCoherence = 0.0f;
    float tensionArc = 0.0f;
    float cadence = 0.0f;
    float loopStrength = 0.0f;
    float voiceLeadingQuality = 0.0f;
    float memorability = 0.0f;
    float vocalSpace = 0.0f;
    float hookStrength = 0.0f;
    float surprise = 0.0f;
    float styleAuthenticity = 0.0f;
    float total = 0.0f;
};

class QualityMetrics
{
public:
    /** Degree-sequence-only evaluation (fast, no voicings). */
    static QualityScores evaluate (const std::array<int, 4>& degrees,
                                   const StyleProfile& style);

    /** Full evaluation with realized voicings (voice leading, hook, loop). */
    static QualityScores evaluateWithPlan (const std::array<int, 4>& degrees,
                                           const StyleProfile& style,
                                           const std::array<ChordRealization, 4>& plan);

    /** Validation thresholds for GOLD bank admission. */
    static constexpr float goldTotalThreshold = 0.62f;
    static constexpr float goldCoherenceFloor = 0.45f;
    static constexpr float goldLoopFloor = 0.45f;

    static bool passesGold (const QualityScores& s)
    {
        return s.total >= goldTotalThreshold
            && s.functionalCoherence >= goldCoherenceFloor
            && s.loopStrength >= goldLoopFloor;
    }
};

} // namespace morph
