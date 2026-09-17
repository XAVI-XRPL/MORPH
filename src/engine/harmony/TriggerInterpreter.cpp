#include "TriggerInterpreter.h"

namespace morph
{

TriggerResult TriggerInterpreter::interpret (MidiPitch input, const KeyContext& key) const
{
    const auto pc = pitchClassOf (input);

    if (auto degree = key.degreeOf (pc))
        return { *degree, false };

    // Chromatic input (STYLE_COLOR policy): map toward the nearest valid
    // scale degree; ties resolve downward. No arbitrary chromatic chords.
    int bestDegree = 1;
    int bestDistance = 12;

    for (int d = 1; d <= key.scale.size; ++d)
    {
        const auto degreePc = key.pitchClassAtDegree (ScaleDegree { d });
        const int up = intervalBetween (pc, degreePc).semitones;       // 0..11
        const int down = intervalBetween (degreePc, pc).semitones;     // 0..11
        const int distance = up < down ? up : down;

        if (distance < bestDistance || (distance == bestDistance && down < up))
        {
            bestDistance = distance;
            bestDegree = d;
        }
    }

    return { ScaleDegree { bestDegree }, true };
}

} // namespace morph
