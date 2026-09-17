#include "TopVoiceEngine.h"
#include <cstdlib>

namespace morph
{

TopVoiceEngine::Candidates TopVoiceEngine::candidatesFor (const ChordSymbol& chord)
{
    Candidates c;
    const auto& f = chord.formula;

    for (int i = 0; i < f.toneCount; ++i)
    {
        const auto pc = transpose (chord.root, Interval { f.semitones[(size_t) i] });
        // Place in every octave, keep those inside the melody register.
        for (int octave = 4; octave <= 5; ++octave)
        {
            const int p = makePitch (pc, octave).value;
            if (p >= registerLow && p <= registerHigh && c.count < (int) c.pitches.size())
                c.pitches[(size_t) c.count++] = p;
        }
    }

    // Ascending sort.
    for (int i = 1; i < c.count; ++i)
    {
        const int key = c.pitches[(size_t) i];
        int j = i - 1;
        while (j >= 0 && c.pitches[(size_t) j] > key)
        {
            c.pitches[(size_t) (j + 1)] = c.pitches[(size_t) j];
            --j;
        }
        c.pitches[(size_t) (j + 1)] = key;
    }
    return c;
}

float TopVoiceEngine::score (int pitch, const TopVoiceContext& context) const
{
    if (! context.hasPrevious)
        return pitch >= 62 && pitch <= 70 ? 2.0f : 1.0f; // centered start

    const int dist = pitch - context.previousTop.value;
    const int adist = std::abs (dist);

    float s = 0.0f;

    // Stepwise motion is the melodic ideal; unison repeats anchor motifs.
    if (adist == 0) s += 3.0f;
    else if (adist <= 2) s += 4.0f;
    else if (adist <= 5) s += 2.0f;
    else if (adist > 7) s -= (float) (adist - 7) * 1.5f; // controlled leaps only

    // Contour: MOTION low → reward staying; high → reward continued direction.
    if (context.motion < 0.4f && adist == 0)
        s += 1.5f;
    if (context.motion > 0.6f && context.previousDirection != 0
        && (dist > 0) == (context.previousDirection > 0) && adist > 0 && adist <= 5)
        s += 1.5f;

    // Register discipline.
    if (pitch < registerLow || pitch > registerHigh)
        s -= 8.0f;

    return s;
}

MidiPitch TopVoiceEngine::chooseTop (const ChordSymbol& chord, const TopVoiceContext& context) const
{
    const auto cands = candidatesFor (chord);
    if (cands.count == 0)
        return MidiPitch { 64 };

    int best = cands.pitches[0];
    float bestScore = -1e9f;

    for (int i = 0; i < cands.count; ++i)
    {
        const float s = score (cands.pitches[(size_t) i], context);
        if (s > bestScore)
        {
            bestScore = s;
            best = cands.pitches[(size_t) i];
        }
    }
    return MidiPitch { best };
}

} // namespace morph
