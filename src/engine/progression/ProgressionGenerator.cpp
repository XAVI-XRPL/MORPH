#include "ProgressionGenerator.h"
#include <algorithm>
#include <cstdlib>

namespace morph
{

namespace
{
    using WeightTable = std::array<std::array<float, 7>, 7>;

    /** Cheap incremental score for one slot step (grammar + arc + VL).
        `wobble` is the per-seed grammar warp used ONLY during search
        (diversity); final metrics use true style weights. */
    float stepScore (const StyleProfile& style, const WeightTable& wobble,
                     int fromDegree, int toDegree, int slot,
                     const VoiceLeadingMemory& mem, const ChordRealization& realized)
    {
        float s = 0.0f;

        // Functional grammar (seed-wobbled for search diversity).
        s += 1.2f * wobble[(size_t) (fromDegree - 1)][(size_t) (toDegree - 1)];

        // Tension arc fit for this slot.
        const float terr = std::abs (StyleProfile::degreeTension (toDegree)
                                     - style.tensionTargets[(size_t) slot]);
        s += 0.5f * (1.0f - terr * 2.0f);

        // Voice-leading increment.
        if (mem.valid)
        {
            float move = 0.0f;
            for (int v = 0; v < realized.voicing.count; ++v)
            {
                int dmin = 24;
                for (int u = 0; u < mem.voicing.count; ++u)
                    dmin = std::min (dmin, std::abs (realized.voicing.voices[(size_t) v].pitch.value
                                                   - mem.voicing.voices[(size_t) u].pitch.value));
                move += (float) std::min (dmin, 12);
                if (dmin == 0)
                    s += 0.06f; // common tone
            }
            move /= (float) realized.voicing.count;
            s += 0.35f * (1.0f - move / 10.0f);
        }
        return s;
    }
}

ProgressionGenerator::Generated
ProgressionGenerator::generate (const StyleProfile& style, const KeyContext& key, uint32_t seed) const
{
    Rng rng { (seed ^ 0x9E3779B9u) | 1u };

    Realizer realizer;
    realizer.key = key;
    realizer.style = style;
    realizer.color = style.brightness;

    // Per-seed grammar warp: 0.8..1.2 per edge — same seed, same flavor.
    std::array<std::array<float, 7>, 7> wobble {};
    for (auto& row : wobble)
        for (auto& w : row)
            w = 0.8f + 0.4f * rng.nextFloat();

    std::vector<BeamState> beam;
    beam.reserve (beamWidth * 8);

    // Slot 0: seeded start degree — mostly tonic home, sometimes a color
    // launch (bVI/iv/bIII are classic R&B openings).
    {
        const float startRoll = (float) (seed % 100u) / 100.0f;
        const int startDegree = startRoll < 0.55f ? 1
                              : startRoll < 0.75f ? 6
                              : startRoll < 0.88f ? 4 : 3;

        BeamState start;
        start.degrees[0] = startDegree;
        start.size = 1;
        VoiceLeadingMemory mem;
        start.lastRealization = realizer.realize (ScaleDegree { startDegree }, mem, 0);
        start.mem = mem;
        start.partialScore = startDegree == 1 ? 0.0f : -0.05f; // anchor bias to tonic
        start.jitter = rng.nextFloat() * 0.12f;
        beam.push_back (start);

        // A tonic-start rival lineage keeps home anchoring competitive.
        if (startDegree != 1)
        {
            BeamState tonic;
            tonic.degrees[0] = 1;
            tonic.size = 1;
            VoiceLeadingMemory m2;
            tonic.lastRealization = realizer.realize (ScaleDegree { 1 }, m2, 0);
            tonic.mem = m2;
            tonic.partialScore = 0.0f;
            tonic.jitter = rng.nextFloat() * 0.12f;
            beam.push_back (tonic);
        }
    }

    // Beam expansion for slots 1..3.
    for (int slot = 1; slot < 4; ++slot)
    {
        std::vector<BeamState> next;
        next.reserve (beam.size() * 7);

        for (const auto& state : beam)
        {
            const int fromDegree = state.degrees[(size_t) (slot - 1)];

            for (int to = 1; to <= 7; ++to)
            {
                BeamState candidate = state;
                candidate.degrees[(size_t) slot] = to;
                candidate.size = slot + 1;

                VoiceLeadingMemory mem = state.mem;
                const auto realized = realizer.realize (ScaleDegree { to }, mem, slot);

                candidate.partialScore += stepScore (style, wobble, fromDegree, to, slot, state.mem, realized);
                candidate.mem = mem;
                candidate.lastRealization = realized;
                next.push_back (candidate);
            }
        }

        // Prune to beam width (deterministic: score + seeded jitter).
        std::sort (next.begin(), next.end(),
                   [] (const BeamState& a, const BeamState& b)
                   { return (a.partialScore + a.jitter) > (b.partialScore + b.jitter); });
        if ((int) next.size() > beamWidth)
            next.resize ((size_t) beamWidth);

        beam = std::move (next);
    }

    // Final full-metric scoring with realized plans.
    Generated best;
    float bestScore = -1e9f;

    for (const auto& state : beam)
    {
        // Realize the full plan from scratch for honest metrics.
        VoiceLeadingMemory mem;
        std::array<ChordRealization, 4> plan {};
        for (int i = 0; i < 4; ++i)
            plan[(size_t) i] = realizer.realize (ScaleDegree { state.degrees[(size_t) i] }, mem, i);

        // Loop closure pass (matches EngineHost::buildProgressionPlan).
        plan[0] = realizer.realize (ScaleDegree { state.degrees[0] }, mem, 0);

        const auto metrics = QualityMetrics::evaluateWithPlan (state.degrees, style, plan);
        const float total = metrics.total + 0.10f * state.partialScore + state.jitter;

        if (total > bestScore)
        {
            bestScore = total;
            best.degrees = state.degrees;
            best.score = total;
            best.metrics = metrics;
        }
    }

    return best;
}

std::vector<ProgressionGenerator::Generated>
ProgressionGenerator::generateMany (const StyleProfile& style, const KeyContext& key,
                                    uint32_t seedBase, int count) const
{
    std::vector<Generated> out;
    out.reserve ((size_t) count);

    for (int i = 0; i < count; ++i)
        out.push_back (generate (style, key, seedBase + (uint32_t) i * 2654435761u));

    return out;
}

} // namespace morph
