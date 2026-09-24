#include "../TestHarness.h"
#include "engine/progression/ProgressionGenerator.h"

using namespace morph;

namespace
{
    const auto key = KeyContext::cMinor();
}

MORPH_TEST (generator, deterministicPerSeed)
{
    // §66/§93: same seed + same parameters = same result.
    ProgressionGenerator gen;
    const auto style = StyleProfile::get (StyleId::modernRnB);

    const auto a = gen.generate (style, key, 777);
    const auto b = gen.generate (style, key, 777);

    for (int i = 0; i < 4; ++i)
        CHECK_EQ (a.degrees[(size_t) i], b.degrees[(size_t) i]);
    CHECK (std::abs (a.score - b.score) < 1e-6f);
}

MORPH_TEST (generator, beatsRandomBaseline)
{
    // §72: sequence-aware generation — generated loops must outscore random
    // degree permutations by a clear margin.
    ProgressionGenerator gen;
    const auto style = StyleProfile::get (StyleId::modernRnB);

    const auto g = gen.generate (style, key, 42);

    float randomTotal = 0.0f;
    uint32_t state = 123456789u;
    for (int i = 0; i < 24; ++i)
    {
        std::array<int, 4> randSeq {};
        for (int slot = 0; slot < 4; ++slot)
        {
            state ^= state << 13; state ^= state >> 17; state ^= state << 5;
            randSeq[(size_t) slot] = 1 + (int) (state % 7u);
        }
        randomTotal += QualityMetrics::evaluate (randSeq, style).total;
    }
    randomTotal /= 24.0f;

    CHECK (g.metrics.total > randomTotal + 0.08f);
    CHECK (QualityMetrics::passesGold (g.metrics));
}

MORPH_TEST (generator, loopResolvesHome)
{
    // Generated loops must resolve: slot 4 → slot 1 transition must be valid.
    ProgressionGenerator gen;
    const auto style = StyleProfile::get (StyleId::modernRnB);

    for (uint32_t seed = 1; seed <= 12; ++seed)
    {
        const auto g = gen.generate (style, key, seed * 97u);
        CHECK (g.metrics.loopStrength >= QualityMetrics::goldLoopFloor);
        CHECK (g.metrics.functionalCoherence >= QualityMetrics::goldCoherenceFloor);
    }
}

MORPH_TEST (generator, stylesProduceDistinctGrammar)
{
    ProgressionGenerator gen;
    const auto rnb = StyleProfile::get (StyleId::modernRnB);
    const auto trap = StyleProfile::get (StyleId::trap);

    bool differ = false;
    for (uint32_t seed = 1; seed <= 6; ++seed)
    {
        const auto a = gen.generate (rnb, key, seed * 1337u);
        const auto b = gen.generate (trap, key, seed * 1337u);
        for (int i = 0; i < 4; ++i)
            if (a.degrees[(size_t) i] != b.degrees[(size_t) i])
                differ = true;
    }
    CHECK (differ);
}

MORPH_TEST (generator, manyDistinctCandidates)
{
    ProgressionGenerator gen;
    const auto style = StyleProfile::get (StyleId::modernRnB);

    const auto batch = gen.generateMany (style, key, 1000, 24);

    // Distinct degree sequences available (not one fixed answer).
    std::vector<std::array<int, 4>> unique;
    for (const auto& g : batch)
        if (std::find (unique.begin(), unique.end(), g.degrees) == unique.end())
            unique.push_back (g.degrees);

    CHECK (unique.size() >= 3);
}
