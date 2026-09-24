#pragma once

#include "QualityMetrics.h"
#include "Realizer.h"
#include "../theory/KeyContext.h"
#include <array>
#include <vector>

namespace morph
{

/**
 * ProgressionGenerator (§72): sequence-aware generation via beam search.
 * beamWidth = 32, branchFactor = 12 (7 diatonic degrees expanded; pruning
 * cap retained for future color variants). Deterministic per seed.
 *
 * Runs on the message thread / tooling — never in the realtime callback.
 */
class ProgressionGenerator
{
public:
    static constexpr int beamWidth = 32;
    static constexpr int branchFactor = 12;

    struct Generated
    {
        std::array<int, 4> degrees {};
        float score = 0.0f;
        QualityScores metrics;
    };

    /** Generate the best 4-slot progression for a style/seed. */
    Generated generate (const StyleProfile& style, const KeyContext& key, uint32_t seed) const;

    /** Generate many distinct candidates (bank tooling). */
    std::vector<Generated> generateMany (const StyleProfile& style, const KeyContext& key,
                                         uint32_t seedBase, int count) const;

private:
    struct BeamState
    {
        std::array<int, 4> degrees {};
        int size = 0;
        float partialScore = 0.0f;
        float jitter = 0.0f;
        VoiceLeadingMemory mem;
        ChordRealization lastRealization;
    };

    struct Rng
    {
        uint32_t state;
        uint32_t next()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return state;
        }
        float nextFloat() { return (float) (next() & 0xFFFFFF) / (float) 0x1000000; }
    };
};

} // namespace morph
