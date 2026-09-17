#include "PerformanceEngine.h"
#include <algorithm>

namespace morph
{

namespace
{
    /** Deterministic xorshift32 — same seed → same humanization everywhere. */
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
        float nextFloat() // 0..1
        {
            return (float) (next() & 0xFFFFFF) / (float) 0x1000000;
        }
        float nextBipolar() { return nextFloat() * 2.0f - 1.0f; }
    };

    float applyCurve (StrumCurve curve, float x)
    {
        switch (curve)
        {
            case StrumCurve::linear:  return x;
            case StrumCurve::easeIn:  return x * x;
            case StrumCurve::easeOut: return 1.0f - (1.0f - x) * (1.0f - x);
            case StrumCurve::human:   return x; // jitter applied by caller
        }
        return x;
    }

    int roleVelocityWeight (VoiceRole role)
    {
        switch (role)
        {
            case VoiceRole::bass:  return -4;
            case VoiceRole::inner: return 0;
            case VoiceRole::top:   return 6;
        }
        return 0;
    }
}

ScheduledNoteList<maxChordVoices>
PerformanceEngine::schedule (const ChordRealization& realization,
                             const PerformanceProfile& profile,
                             double sampleRate,
                             int64_t noteLengthSamples,
                             uint32_t seed)
{
    ScheduledNoteList<maxChordVoices> out;
    Rng rng { seed | 1u };

    const auto& voicing = realization.voicing;
    const int n = voicing.count;
    if (n == 0 || ! realization.valid)
        return out;

    const float humanT = profile.humanizeTiming;
    const float humanV = profile.humanizeVelocity;

    if (profile.mode == PerformanceMode::together)
    {
        // TEXTURE loosens the ensemble slightly, always inside tolerance.
        const float maxOffMs = profile.together.maxOffsetMs() * (1.0f + humanT);

        for (int i = 0; i < n; ++i)
        {
            const auto& v = voicing.voices[(size_t) i];
            const float offMs = (maxOffMs > 0.0f)
                ? (rng.nextFloat() * maxOffMs) : 0.0f;

            ScheduledNote sn;
            sn.pitch = v.pitch.value;
            sn.velocity = std::clamp (
                profile.velocityBaseline + roleVelocityWeight (v.role)
                + (int) (rng.nextBipolar() * 4.0f * (0.5f + humanV)), 1, 127);
            sn.noteOnSampleOffset = (int64_t) (offMs * 0.001 * sampleRate);
            sn.noteOffSampleOffset = noteLengthSamples;
            sn.role = v.role;
            out.add (sn);
        }
        return out;
    }

    // --- STRUM ---
    const auto& sp = profile.strum;
    const bool up = (profile.mode == PerformanceMode::strumUp);

    // 1. Pitch-ordered working set.
    std::array<Voice, Voicing::maxVoices> ordered {};
    int m = 0;
    for (int i = 0; i < n; ++i)
        ordered[(size_t) m++] = voicing.voices[(size_t) i];
    // insertion sort by pitch, ascending
    for (int i = 1; i < m; ++i)
    {
        auto key = ordered[(size_t) i];
        int j = i - 1;
        while (j >= 0 && ordered[(size_t) j].pitch.value > key.pitch.value)
        {
            ordered[(size_t) (j + 1)] = ordered[(size_t) j];
            --j;
        }
        ordered[(size_t) (j + 1)] = key;
    }

    // 2. Extract bass / top per policy.
    auto findRole = [&] (VoiceRole role) -> int
    {
        for (int i = 0; i < m; ++i)
            if (ordered[(size_t) i].role == role)
                return i;
        return -1;
    };

    Voice bassVoice { MidiPitch { -1 }, VoiceRole::bass };
    Voice topVoice  { MidiPitch { -1 }, VoiceRole::top };
    bool bassExtracted = false, topExtracted = false;

    auto removeAt = [&] (int idx)
    {
        for (int i = idx; i < m - 1; ++i)
            ordered[(size_t) i] = ordered[(size_t) (i + 1)];
        --m;
    };

    if (sp.bassPolicy != BassStrumPolicy::withStrum)
    {
        const int bi = findRole (VoiceRole::bass);
        if (bi >= 0) { bassVoice = ordered[(size_t) bi]; removeAt (bi); bassExtracted = true; }
    }
    if (sp.topVoicePolicy == TopVoicePerformancePolicy::arriveLast
        || sp.topVoicePolicy == TopVoicePerformancePolicy::arriveFirst
        || sp.topVoicePolicy == TopVoicePerformancePolicy::accent
        || sp.topVoicePolicy == TopVoicePerformancePolicy::hold)
    {
        const int ti = findRole (VoiceRole::top);
        if (ti >= 0) { topVoice = ordered[(size_t) ti]; removeAt (ti); topExtracted = true; }
    }

    // 3. Remaining voices in strum direction order.
    std::array<Voice, Voicing::maxVoices> strummed {};
    int sCount = 0;
    for (int i = 0; i < m; ++i)
        strummed[(size_t) sCount++] = up ? ordered[(size_t) i]
                                         : ordered[(size_t) (m - 1 - i)];

    // 4. Assemble final attack order.
    std::array<Voice, Voicing::maxVoices + 1> order {};
    int oCount = 0;

    const bool bassFirst = bassExtracted
        && (sp.bassPolicy == BassStrumPolicy::anchorFirst
            || sp.bassPolicy == BassStrumPolicy::anchorSimultaneous);

    if (bassFirst)
        order[(size_t) oCount++] = bassVoice;

    if (topExtracted && sp.topVoicePolicy == TopVoicePerformancePolicy::arriveFirst)
        order[(size_t) oCount++] = topVoice;

    for (int i = 0; i < sCount; ++i)
        order[(size_t) oCount++] = strummed[(size_t) i];

    if (topExtracted && (sp.topVoicePolicy == TopVoicePerformancePolicy::arriveLast
                         || sp.topVoicePolicy == TopVoicePerformancePolicy::accent
                         || sp.topVoicePolicy == TopVoicePerformancePolicy::hold))
        order[(size_t) oCount++] = topVoice;

    if (bassExtracted && sp.bassPolicy == BassStrumPolicy::delayed)
        order[(size_t) oCount++] = bassVoice;

    // (excluded: bass never re-enters the order)

    // 5. Positions across the spread.
    const int positions = oCount;
    for (int i = 0; i < positions; ++i)
    {
        float x = positions > 1 ? (float) i / (float) (positions - 1) : 0.0f;

        // AnchorSimultaneous: the first strummed voice shares t = 0 with bass.
        if (sp.bassPolicy == BassStrumPolicy::anchorSimultaneous && bassFirst && i == 1)
            x = 0.0f;

        float eased = applyCurve (sp.curve, x);

        if (sp.curve == StrumCurve::human)
            eased = x + rng.nextBipolar() * 0.06f * sp.randomTimingAmount;

        eased = std::clamp (eased, 0.0f, 1.0f);

        const float offMs = eased * sp.spreadMs;

        // Velocity: baseline + role weight + shape + humanization.
        float shape = 1.0f;
        if (sp.velocityShape == StrumVelocityShape::rise)
            shape = 0.88f + 0.22f * x;
        else if (sp.velocityShape == StrumVelocityShape::fall)
            shape = 1.10f - 0.22f * x;

        int velocity = (int) (profile.velocityBaseline * shape)
                     + roleVelocityWeight (order[(size_t) i].role)
                     + (int) (rng.nextBipolar() * 6.0f * sp.randomVelocityAmount);

        if (topExtracted && order[(size_t) i].role == VoiceRole::top
            && sp.topVoicePolicy == TopVoicePerformancePolicy::accent)
            velocity += 12;

        int64_t length = noteLengthSamples;
        if (topExtracted && order[(size_t) i].role == VoiceRole::top
            && sp.topVoicePolicy == TopVoicePerformancePolicy::hold
            && noteLengthSamples > 0)
            length = noteLengthSamples + noteLengthSamples / 2;

        ScheduledNote sn;
        sn.pitch = order[(size_t) i].pitch.value;
        sn.velocity = std::clamp (velocity, 1, 127);
        sn.noteOnSampleOffset = (int64_t) (offMs * 0.001 * sampleRate);
        sn.noteOffSampleOffset = length;
        sn.role = order[(size_t) i].role;
        out.add (sn);
    }

    // Human curve: enforce monotonic non-decreasing attack times.
    for (int i = 1; i < out.count; ++i)
        if (out.notes[(size_t) i].noteOnSampleOffset < out.notes[(size_t) (i - 1)].noteOnSampleOffset)
            out.notes[(size_t) i].noteOnSampleOffset = out.notes[(size_t) (i - 1)].noteOnSampleOffset;

    return out;
}

} // namespace morph
