#include "../TestHarness.h"
#include "engine/bank/ProgressionBank.h"
#include "engine/progression/ProgressionGenerator.h"
#include <set>

using namespace morph;

MORPH_TEST (bank, has300GoldEntries)
{
    // §100: start with 300 GOLD seeds.
    CHECK (goldBankCount >= 300);

    for (int s = 0; s < numImplementedStyles; ++s)
    {
        int count = 0;
        bankEntriesForStyle ((StyleId) s, count);
        CHECK (count >= 50);
    }
}

MORPH_TEST (bank, everyEntryPassesGoldThresholds)
{
    // Bank validation: re-run each entry through the generator and re-check.
    ProgressionGenerator gen;
    const auto key = KeyContext::cMinor();

    for (int i = 0; i < goldBankCount; ++i)
    {
        const auto& e = goldBank[i];
        const auto style = StyleProfile::get (e.style);
        const auto g = gen.generate (style, key, e.seed * 2654435761u + 17u);

        // The stored seed regenerates the stored degrees (determinism, §93).
        for (int slot = 0; slot < 4; ++slot)
            CHECK_EQ ((int) e.degrees[(size_t) slot], g.degrees[(size_t) slot]);

        CHECK (QualityMetrics::passesGold (g.metrics));
    }
}

MORPH_TEST (bank, noDuplicateSequencesPerStyle)
{
    for (int s = 0; s < numImplementedStyles; ++s)
    {
        int count = 0;
        const auto* entries = bankEntriesForStyle ((StyleId) s, count);
        CHECK (entries != nullptr);

        std::set<uint32_t> seen;
        for (int i = 0; i < count; ++i)
        {
            const auto& d = entries[i].degrees;
            const uint32_t h = (uint32_t) (d[0] * 1000 + d[1] * 100 + d[2] * 10 + d[3]);
            CHECK (seen.insert (h).second); // deduplication proof
        }
    }
}
