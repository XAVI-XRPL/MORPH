#include "BassEngine.h"
#include <cstdlib>

namespace morph
{

namespace
{
    /** Place a pitch class into the bass register window [36..47]. */
    int inBassRegister (PitchClass pc)
    {
        int p = makePitch (pc, 2).value; // 36 + pc
        while (p < 36) p += 12;
        while (p > 47) p -= 12;
        return p;
    }
}

BassMotionProfile BassEngine::profileForMotion (float motion)
{
    if (motion < 0.33f) return BassMotionProfile::root;
    if (motion < 0.66f) return BassMotionProfile::flow;
    return BassMotionProfile::bounce;
}

MidiPitch BassEngine::chooseBass (const ChordSymbol& chord,
                                  const BassContext& context) const
{
    const auto profile = context.profileOverride >= 0
        ? (BassMotionProfile) context.profileOverride
        : profileForMotion (context.motion);

    switch (profile)
    {
        case BassMotionProfile::pedal:
            return MidiPitch { inBassRegister (context.tonalCenter) };

        case BassMotionProfile::bounce:
        {
            // Root on even slots, fifth on odd slots (falls back to root).
            const bool useFifth = (context.slotIndex % 2 == 1);
            const int fifthOffset = chord.formula.toneCount > 2
                                        ? chord.formula.semitones[2] : 7;
            const auto pc = transpose (chord.root, Interval { useFifth ? fifthOffset : 0 });
            return MidiPitch { inBassRegister (pc) };
        }

        case BassMotionProfile::flow:
        {
            if (! context.hasPrevious)
                return MidiPitch { inBassRegister (chord.root) };

            // Smoothest chord-tone bass among root / third / fifth. When the
            // bass would park on a pitch that is NOT the new chord's root,
            // move by step instead — the bass stays melodic (§50).
            const bool parkedOnNonRoot =
                pitchClassOf (context.previousBass) != chord.root;

            int bestPitch = inBassRegister (chord.root);
            int bestDist = std::abs (bestPitch - context.previousBass.value);

            for (int i = 1; i < chord.formula.toneCount && i < 3; ++i)
            {
                const auto pc = transpose (chord.root, Interval { chord.formula.semitones[(size_t) i] });
                const int p = inBassRegister (pc);
                const int dist = std::abs (p - context.previousBass.value);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestPitch = p;
                }
            }

            if (parkedOnNonRoot && bestPitch == context.previousBass.value)
            {
                // Re-pick the closest chord tone that is NOT the parked pitch.
                int altPitch = -1, altDist = 24;
                for (int i = 0; i < chord.formula.toneCount && i < 3; ++i)
                {
                    const auto pc = transpose (chord.root, Interval { chord.formula.semitones[(size_t) i] });
                    const int p = inBassRegister (pc);
                    if (p == context.previousBass.value)
                        continue;
                    const int dist = std::abs (p - context.previousBass.value);
                    if (dist < altDist)
                    {
                        altDist = dist;
                        altPitch = p;
                    }
                }
                if (altPitch >= 0 && altDist <= 5)
                    bestPitch = altPitch;
            }

            return MidiPitch { bestPitch };
        }

        case BassMotionProfile::root:
        default:
            return MidiPitch { inBassRegister (chord.root) };
    }
}

} // namespace morph
