#pragma once

#include "../theory/Chord.h"
#include "../theory/KeyContext.h"
#include <array>

namespace morph
{

struct TopVoiceContext
{
    MidiPitch previousTop { -1 };
    bool hasPrevious = false;
    int previousDirection = 0; // -1 falling, 0 static, +1 rising (contour memory)
    float motion = 0.5f;       // MOTION knob: still favors repetition/stepwise
};

/**
 * TopVoiceEngine (§51): the top voice is melodic — never highestChordTone().
 * Chooses among chord tones in the melody register [60..76], preferring
 * stepwise motion, motif repetition, and controlled leaps.
 */
class TopVoiceEngine
{
public:
    static constexpr int registerLow = 60;
    static constexpr int registerHigh = 76;

    /** All legal chord-tone placements in the melody register, ascending. */
    struct Candidates
    {
        std::array<int, 12> pitches {};
        int count = 0;
    };
    static Candidates candidatesFor (const ChordSymbol& chord);

    MidiPitch chooseTop (const ChordSymbol& chord, const TopVoiceContext& context) const;

    /** Score a candidate top against context (higher = better). Exposed for
        VoicingEngine's candidate scoring. */
    float score (int pitch, const TopVoiceContext& context) const;
};

} // namespace morph
