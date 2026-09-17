#include "VoicingEngine.h"

namespace morph
{

namespace
{
    /** True if `target` semitone offset is present in the formula. */
    bool hasTone (const ChordFormula& f, int semitone)
    {
        for (int i = 0; i < f.toneCount; ++i)
            if (f.semitones[(size_t) i] == semitone)
                return true;
        return false;
    }

    int thirdOf (const ChordFormula& f)
    {
        // Returns the semitone offset of the "third-area" tone, or -1 for sus.
        if (hasTone (f, 3)) return 3;
        if (hasTone (f, 4)) return 4;
        return -1;
    }

    /** Place a chord tone (semitone offset from root) in a target octave,
        then lift by octaves until it sits strictly above `floor`. */
    int placeAbove (PitchClass root, int semitoneOffset, int octave, int floor)
    {
        const auto pc = transpose (root, Interval { semitoneOffset });
        int pitch = makePitch (pc, octave).value;
        while (pitch <= floor && pitch <= 115)
            pitch += 12;
        return pitch;
    }
}

ChordRealization VoicingEngine::realize (const ChordCandidate& candidate,
                                         const KeyContext& key,
                                         const StyleProfile& style,
                                         const Voicing* previous,
                                         float openness,
                                         int variation) const
{
    (void) key;

    ChordRealization out;
    out.candidate = candidate;

    const auto& f = candidate.chord.formula;
    const auto root = candidate.chord.root;

    // --- Bass: single root in the style's bass octave (low-end cleanliness) ---
    int bass = makePitch (root, style.preferredBassOctave).value; // [36..47] for octave 2
    while (bass < 36) bass += 12;
    while (bass > 47) bass -= 12;

    // --- Low inner: fifth (or b5) directly above the bass ---
    const int fifthOffset = hasTone (f, 6) ? 6 : 7;
    const int lowInner = bass + fifthOffset;

    // --- Mid inner: seventh in octave 3, above the low inner ---
    int midInner = -1;
    if (hasTone (f, 10))      midInner = placeAbove (root, 10, 3, lowInner + 1);
    else if (hasTone (f, 11)) midInner = placeAbove (root, 11, 3, lowInner + 1);

    // --- High inner: ninth, then eleventh in octave 4 ---
    int highInner = -1;
    if (hasTone (f, 14))      highInner = placeAbove (root, 14, 4, midInner > 0 ? midInner : lowInner);

    int eleventh = -1;
    if (hasTone (f, 17))      eleventh = placeAbove (root, 17, 4, highInner > 0 ? highInner : (midInner > 0 ? midInner : lowInner));

    // --- Top: third (or fourth for sus) above everything so far ---
    const int thirdOffset = thirdOf (f) >= 0 ? thirdOf (f) : (hasTone (f, 5) ? 5 : 2);
    const int topFloor = eleventh > 0 ? eleventh
                       : highInner > 0 ? highInner
                       : midInner  > 0 ? midInner : lowInner;
    int top = placeAbove (root, thirdOffset, 4, topFloor);

    // Top-voice continuity: prefer the octave nearest the previous top.
    if (previous != nullptr && previous->count > 0)
    {
        const int prevTop = previous->highest().value;
        const int down = top - 12;
        if (down > topFloor && down >= 58
            && std::abs (down - prevTop) < std::abs (top - prevTop))
            top = down;
    }

    // --- SPACE openness ---
    if (openness > 0.66f)
    {
        if (highInner > 0) highInner += 12;
        top += 12;
    }
    else if (openness < 0.33f)
    {
        const int lowered = top - 12;
        if (lowered > topFloor && lowered >= 58)
            top = lowered;
    }

    // --- MORPH action: deterministic voicing sibling (interim, pre-M5) ---
    // Same chord identity and roles, altered shape. Never random.
    if (variation != 0)
    {
        const int v = ((variation - 1) % 3 + 3) % 3;

        if (v == 0)
        {
            // Lift the top voice an octave (brighter melody line).
            if (top + 12 <= 86)
                top += 12;
            else if (midInner > 0 && midInner - 12 > lowInner)
                midInner -= 12;
        }
        else if (v == 1)
        {
            // Open the upper structure (9th and top lift together).
            if (highInner > 0 && top + 12 <= 86)
            {
                highInner += 12;
                top += 12;
            }
            else if (midInner > 0 && midInner - 12 > lowInner)
            {
                midInner -= 12;
            }
        }
        else
        {
            // Drop the seventh into the lower octave (darker color).
            if (midInner > 0 && midInner - 12 > lowInner)
                midInner -= 12;
            else if (top + 12 <= 86)
                top += 12;
        }
    }

    // --- Assemble (ascending) ---
    Voicing v;
    v.add (Voice { MidiPitch { bass }, VoiceRole::bass });
    v.add (Voice { MidiPitch { lowInner }, VoiceRole::inner });
    if (midInner > 0)  v.add (Voice { MidiPitch { midInner }, VoiceRole::inner });
    if (highInner > 0) v.add (Voice { MidiPitch { highInner }, VoiceRole::inner });
    if (eleventh > 0)  v.add (Voice { MidiPitch { eleventh }, VoiceRole::inner });
    v.add (Voice { MidiPitch { top }, VoiceRole::top });
    v.sortAscending();

    // Re-assign roles after sorting: lowest = bass, highest = top.
    for (int i = 0; i < v.count; ++i)
        v.voices[(size_t) i].role = (i == 0) ? VoiceRole::bass
                                  : (i == v.count - 1) ? VoiceRole::top
                                  : VoiceRole::inner;

    out.voicing = v;
    out.bassPitch = v.lowest();
    out.topPitch = v.highest();
    out.valid = true;
    return out;
}

} // namespace morph
