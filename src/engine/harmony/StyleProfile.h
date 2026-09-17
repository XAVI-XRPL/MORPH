#pragma once

#include "../theory/Chord.h"
#include "../performance/PerformanceTypes.h"

namespace morph
{

enum class StyleId
{
    modernRnB = 0,
    darkRnB,
    neoSoul,
    emotional,
    darkPop,
    trap,
    reggaeton
    // V1 ships Modern R&B; the rest are reserved profile slots.
};

/**
 * StyleProfile: the vocabulary and behavior of a musical style.
 * V1 implements Modern R&B fully; other styles are declared but fall back
 * to the Modern R&B vocabulary until their milestones land.
 */
struct StyleProfile
{
    StyleId id = StyleId::modernRnB;

    // Recommended performance behavior (explicit user selection wins).
    BassStrumPolicy recommendedBassPolicy = BassStrumPolicy::anchorFirst;
    float recommendedStrumSpreadMs = 42.0f;

    int preferredBassOctave = 2;       // bass root placement octave
    bool preferNinthChords = true;

    static StyleProfile modernRnB();
};

} // namespace morph
