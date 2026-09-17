#include "VoicingEngine.h"
#include <cstdlib>

namespace morph
{

namespace
{
    bool hasTone (const ChordFormula& f, int semitone)
    {
        for (int i = 0; i < f.toneCount; ++i)
            if (f.semitones[(size_t) i] == semitone)
                return true;
        return false;
    }

    int thirdOf (const ChordFormula& f)
    {
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

    void assignRoles (Voicing& v)
    {
        v.sortAscending();
        for (int i = 0; i < v.count; ++i)
            v.voices[(size_t) i].role = (i == 0) ? VoiceRole::bass
                                      : (i == v.count - 1) ? VoiceRole::top
                                      : VoiceRole::inner;
    }
}

/** Builds the inner voices around a given bass and top (register bands). */
static Voicing buildWithBassAndTop (const ChordSymbol& chord, int bass, int top)
{
    const auto& f = chord.formula;
    const auto root = chord.root;

    Voicing v;
    v.add (Voice { MidiPitch { bass }, VoiceRole::bass });

    // Low inner: fifth (or b5) directly above the bass.
    const int fifthOffset = hasTone (f, 6) ? 6 : 7;
    const int lowInner = bass + fifthOffset;
    if (lowInner < top - 2)
        v.add (Voice { MidiPitch { lowInner }, VoiceRole::inner });

    // Mid inner: seventh, folded below the top.
    if (hasTone (f, 10) || hasTone (f, 11))
    {
        const int seventhOffset = hasTone (f, 10) ? 10 : 11;
        int midInner = placeAbove (root, seventhOffset, 3, lowInner + 1);
        while (midInner >= top - 1 && midInner > lowInner + 12)
            midInner -= 12;
        if (midInner > lowInner && midInner < top - 1)
            v.add (Voice { MidiPitch { midInner }, VoiceRole::inner });
    }

    // High inner: ninth, then eleventh, folded below the top.
    if (hasTone (f, 14))
    {
        int highInner = placeAbove (root, 14, 4, lowInner);
        while (highInner >= top - 1 && highInner > lowInner + 12)
            highInner -= 12;
        if (highInner > lowInner && highInner < top - 1)
        {
            bool dup = false;
            for (int i = 0; i < v.count; ++i)
                if (v.voices[(size_t) i].pitch.value == highInner)
                    dup = true;
            if (! dup)
                v.add (Voice { MidiPitch { highInner }, VoiceRole::inner });
        }
    }

    if (hasTone (f, 17))
    {
        int eleventh = placeAbove (root, 17, 4, lowInner);
        while (eleventh >= top - 1 && eleventh > lowInner + 12)
            eleventh -= 12;
        if (eleventh > lowInner && eleventh < top - 1)
        {
            bool dup = false;
            for (int i = 0; i < v.count; ++i)
                if (v.voices[(size_t) i].pitch.value == eleventh)
                    dup = true;
            if (! dup)
                v.add (Voice { MidiPitch { eleventh }, VoiceRole::inner });
        }
    }

    v.add (Voice { MidiPitch { top }, VoiceRole::top });
    assignRoles (v);
    return v;
}

/** The canonical M1/M2 recipe (golden Cm9 = C2 G2 Bb3 D4 Eb4). */
static Voicing canonicalRecipe (const ChordSymbol& chord, float openness)
{
    const auto& f = chord.formula;
    const auto root = chord.root;

    int bass = makePitch (root, 2).value;
    while (bass < 36) bass += 12;
    while (bass > 47) bass -= 12;

    const int fifthOffset = hasTone (f, 6) ? 6 : 7;
    const int lowInner = bass + fifthOffset;

    int midInner = -1;
    if (hasTone (f, 10))      midInner = placeAbove (root, 10, 3, lowInner + 1);
    else if (hasTone (f, 11)) midInner = placeAbove (root, 11, 3, lowInner + 1);

    int highInner = -1;
    if (hasTone (f, 14))      highInner = placeAbove (root, 14, 4, midInner > 0 ? midInner : lowInner);

    int eleventh = -1;
    if (hasTone (f, 17))      eleventh = placeAbove (root, 17, 4, highInner > 0 ? highInner : (midInner > 0 ? midInner : lowInner));

    const int thirdOffset = thirdOf (f) >= 0 ? thirdOf (f) : (hasTone (f, 5) ? 5 : 2);
    const int topFloor = eleventh > 0 ? eleventh
                       : highInner > 0 ? highInner
                       : midInner  > 0 ? midInner : lowInner;
    int top = placeAbove (root, thirdOffset, 4, topFloor);

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

    Voicing v;
    v.add (Voice { MidiPitch { bass }, VoiceRole::bass });
    v.add (Voice { MidiPitch { lowInner }, VoiceRole::inner });
    if (midInner > 0)  v.add (Voice { MidiPitch { midInner }, VoiceRole::inner });
    if (highInner > 0) v.add (Voice { MidiPitch { highInner }, VoiceRole::inner });
    if (eleventh > 0)  v.add (Voice { MidiPitch { eleventh }, VoiceRole::inner });
    v.add (Voice { MidiPitch { top }, VoiceRole::top });
    assignRoles (v);
    return v;
}

void VoicingEngine::applyVariation (Voicing& v, int variation)
{
    if (variation == 0 || v.count < 3)
        return;

    const int mode = ((variation - 1) % 3 + 3) % 3;
    // Work on sorted voices: index 0 = bass, last = top.
    if (mode == 0)
    {
        // Lift the top voice an octave (brighter melody line).
        auto& top = v.voices[(size_t) (v.count - 1)];
        if (top.pitch.value + 12 <= 86)
            top.pitch.value += 12;
    }
    else if (mode == 1)
    {
        // Open the upper structure (top two voices lift).
        if (v.count >= 4)
        {
            auto& hi = v.voices[(size_t) (v.count - 2)];
            auto& top = v.voices[(size_t) (v.count - 1)];
            if (hi.pitch.value + 12 <= 84 && top.pitch.value + 12 <= 86)
            {
                hi.pitch.value += 12;
                top.pitch.value += 12;
            }
        }
    }
    else
    {
        // Drop the highest inner voice an octave (darker color).
        if (v.count >= 4)
        {
            auto& hi = v.voices[(size_t) (v.count - 2)];
            const int lowered = hi.pitch.value - 12;
            if (lowered > v.voices[1].pitch.value) // stays above the low inner
                hi.pitch.value = lowered;
        }
    }
    assignRoles (v);
}

ChordRealization VoicingEngine::realize (const ChordCandidate& candidate,
                                         const KeyContext& key,
                                         const StyleProfile& style,
                                         const VoicingContext& context,
                                         int variation) const
{
    (void) key;
    (void) style;

    ChordRealization out;
    out.candidate = candidate;

    // --- No previous context: canonical recipe (golden path) ---
    if (context.previous == nullptr || context.previous->count == 0)
    {
        auto v = canonicalRecipe (candidate.chord, context.openness);
        applyVariation (v, variation);
        out.voicing = v;
        out.bassPitch = v.lowest();
        out.topPitch = v.highest();
        out.valid = true;
        return out;
    }

    // --- M3: candidate generation + voice-leading scoring (§48) ---

    // Bass options: BassEngine's melodic choice + root fallback.
    BassContext bassCtx;
    bassCtx.previousBass = context.previousBass;
    bassCtx.hasPrevious = context.hasPreviousBass;
    bassCtx.tonalCenter = context.tonalCenter;
    bassCtx.slotIndex = context.slotIndex;
    bassCtx.motion = context.motion;

    std::array<int, 2> bassOptions {};
    int bassCount = 0;
    bassOptions[(size_t) bassCount++] = bassEngine.chooseBass (candidate.chord, bassCtx).value;
    {
        int rootBass = makePitch (candidate.chord.root, 2).value;
        while (rootBass < 36) rootBass += 12;
        while (rootBass > 47) rootBass -= 12;
        if (rootBass != bassOptions[0])
            bassOptions[(size_t) bassCount++] = rootBass;
    }

    // Top options: canonical recipe top + best TopVoiceEngine choices.
    TopVoiceContext topCtx;
    topCtx.previousTop = context.previousTop;
    topCtx.hasPrevious = context.hasPreviousTop;
    topCtx.previousDirection = context.previousTopDirection;
    topCtx.motion = context.motion;

    std::array<int, 4> topOptions {};
    int topCount = 0;
    topOptions[(size_t) topCount++] = topVoiceEngine.chooseTop (candidate.chord, topCtx).value;

    {
        const auto canonical = canonicalRecipe (candidate.chord, context.openness);
        const int canonicalTop = canonical.highest().value;
        bool dup = false;
        for (int i = 0; i < topCount; ++i) if (topOptions[(size_t) i] == canonicalTop) dup = true;
        if (! dup) topOptions[(size_t) topCount++] = canonicalTop;
    }
    {
        const auto cands = TopVoiceEngine::candidatesFor (candidate.chord);
        for (int i = 0; i < cands.count && topCount < 4; ++i)
        {
            bool dup = false;
            for (int j = 0; j < topCount; ++j) if (topOptions[(size_t) j] == cands.pitches[(size_t) i]) dup = true;
            if (! dup) topOptions[(size_t) topCount++] = cands.pitches[(size_t) i];
        }
    }

    // Generate + score candidates.
    float bestScore = -1e9f;
    Voicing best {};

    const Voicing& prev = *context.previous;

    for (int b = 0; b < bassCount; ++b)
        for (int t = 0; t < topCount; ++t)
        {
            const int bass = bassOptions[(size_t) b];
            const int top = topOptions[(size_t) t];
            if (top <= bass + 12)
                continue;

            auto v = buildWithBassAndTop (candidate.chord, bass, top);
            if (v.count < 3)
                continue;

            // --- scoring (§48) ---
            float s = 0.0f;

            // Voice movement: nearest-neighbor distance to previous voices.
            for (int i = 0; i < v.count; ++i)
            {
                const int p = v.voices[(size_t) i].pitch.value;
                int dmin = 24;
                for (int j = 0; j < prev.count; ++j)
                    dmin = std::min (dmin, std::abs (p - prev.voices[(size_t) j].pitch.value));

                s -= (float) std::min (dmin, 12);
                if (dmin == 0)
                    s += 2.0f; // common tone
                else if (dmin > 7)
                    s -= 2.0f * (float) (dmin - 7); // large leap penalty
            }

            // Top-line quality (weighted heavily — the melody identity).
            s += 2.0f * topVoiceEngine.score (top, topCtx);

            // Bass quality: stepwise/unison movement, gentle root bonus.
            if (context.hasPreviousBass)
            {
                const int bd = std::abs (bass - context.previousBass.value);
                if (bd == 0) s += 2.5f;
                else if (bd <= 2) s += 2.0f;
                else if (bd > 7) s -= 1.5f;
            }
            if (pitchClassOf (MidiPitch { bass }) == candidate.chord.root)
                s += 0.5f;

            // Register: top in the melody window.
            if (top >= TopVoiceEngine::registerLow && top <= TopVoiceEngine::registerHigh)
                s += 1.0f;
            else
                s -= 6.0f;

            // Spacing: punish inner clusters and huge gaps (skip bass pair).
            for (int i = 2; i < v.count; ++i)
            {
                const int gap = v.voices[(size_t) i].pitch.value - v.voices[(size_t) (i - 1)].pitch.value;
                if (gap < 3)  s -= 3.0f;
                if (gap > 14) s -= 1.5f;
            }

            if (s > bestScore)
            {
                bestScore = s;
                best = v;
            }
        }

    if (best.count == 0)
        best = canonicalRecipe (candidate.chord, context.openness);

    applyVariation (best, variation);

    out.voicing = best;
    out.bassPitch = best.lowest();
    out.topPitch = best.highest();
    out.valid = true;
    return out;
}

} // namespace morph
