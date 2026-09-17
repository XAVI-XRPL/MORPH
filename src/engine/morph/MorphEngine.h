#pragma once

#include "../progression/Progression.h"
#include "../theory/KeyContext.h"
#include <cstdint>

namespace morph
{

/**
 * MorphEngine (§66): generates a musical sibling of the current progression.
 *
 * Preserves: key, style, bars, locks, general harmonic DNA (slot count,
 * anchor degree of slot 1).
 * Changes (unlocked slots only, probability scaled by morphAmount):
 * functional substitutions from the style's relationship tables.
 *
 * Deterministic: same seed + same parameters → same sibling (§66, §93).
 * Never random: substitution tables are stylistic, not noise.
 */
class MorphEngine
{
public:
    /**
     * @param current     the progression to morph
     * @param morphAmount 0..1 (MORPH knob): probability per unlocked slot
     * @param seed        deterministic seed (e.g. morph counter)
     * @return the sibling progression; locked slots identical to input
     */
    Progression morphProgression (const Progression& current,
                                  float morphAmount,
                                  uint32_t seed) const;

private:
    /** Deterministic xorshift32. */
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
