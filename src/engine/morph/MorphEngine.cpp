#include "MorphEngine.h"

namespace morph
{

Progression MorphEngine::morphProgression (const Progression& current,
                                           float morphAmount,
                                           uint32_t seed) const
{
    // Modern R&B minor functional substitution table.
    // Each degree maps to stylistic relatives (diatonic, contextually valid).
    static const int relatives[8][3] =
    {
        { 0, 0, 0 },    // unused (degrees are 1-based)
        { 6, 3, 1 },    // i   → bVI / bIII (tonic area)
        { 4, 7, 2 },    // iiø → iv / bVII
        { 1, 6, 3 },    // bIII → i / bVI
        { 2, 1, 4 },    // iv  → iiø / i
        { 7, 3, 5 },    // V   → bVII / bIII
        { 3, 4, 6 },    // bVI → bIII / iv
        { 5, 1, 7 },    // bVII → V / i
    };

    Progression out = current;
    Rng rng { seed | 1u };
    bool anyChanged = false;
    int firstUnlocked = -1;

    for (int i = 0; i < current.size; ++i)
    {
        const auto& slot = current.slots[(size_t) i];
        if (slot.locked)
            continue; // locked material never mutates (§67, §109.10)

        if (firstUnlocked < 0)
            firstUnlocked = i;

        // Slot 1 is the tonic anchor — morphing it changes identity, so the
        // anchor only morphs at high morph amounts.
        if (i == 0 && morphAmount < 0.75f)
            continue;

        const float roll = rng.nextFloat();
        if (roll >= morphAmount)
            continue; // this slot stays

        const int degree = slot.degree.value;
        const int choice = (int) (rng.nextFloat() * 2.999f); // 0..2
        const int sub = relatives[degree & 7][choice];

        if (sub != degree)
        {
            out.slots[(size_t) i].degree = ScaleDegree { sub };
            anyChanged = true;
        }
    }

    // A MORPH press must evolve the progression: if the dice kept everything
    // (possible at low morph amounts), deterministically morph one slot.
    if (! anyChanged && firstUnlocked >= 0)
    {
        int target = firstUnlocked;
        if (target == 0 && morphAmount < 0.75f)
        {
            target = -1;
            for (int i = 1; i < current.size; ++i)
                if (! current.slots[(size_t) i].locked) { target = i; break; }
        }
        if (target >= 0)
        {
            const int degree = current.slots[(size_t) target].degree.value;
            const int choice = (int) (rng.nextFloat() * 2.999f);
            int sub = relatives[degree & 7][choice];
            // The third table entry is "stay"; force-change must actually move.
            if (sub == degree)
                sub = relatives[degree & 7][(choice + 1) % 3];
            if (sub == degree)
                sub = relatives[degree & 7][(choice + 2) % 3];
            out.slots[(size_t) target].degree = ScaleDegree { sub };
        }
    }

    return out;
}

} // namespace morph
