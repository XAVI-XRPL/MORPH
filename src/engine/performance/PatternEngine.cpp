#include "PatternEngine.h"
#include <algorithm>
#include <cstdlib>

namespace morph
{

namespace
{
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
        float nextBipolar() { return ((float) (next() & 0xFFFFFF) / (float) 0x800000) - 1.0f; }
    };

    int roleWeight (VoiceRole r)
    {
        return r == VoiceRole::bass ? -4 : r == VoiceRole::top ? 6 : 0;
    }

    ScheduledNote makeNote (int pitch, int velocity, int64_t on, int64_t length, VoiceRole role)
    {
        ScheduledNote n;
        n.pitch = pitch;
        n.velocity = std::clamp (velocity, 1, 127);
        n.noteOnSampleOffset = on;
        n.noteOffSampleOffset = on + std::max<int64_t> (1, length);
        n.role = role;
        return n;
    }
}

int64_t PatternEngine::rateToSamples (StreamRate rate, double sampleRate, double bpm)
{
    const double quarter = sampleRate * 60.0 / bpm;
    switch (rate)
    {
        case StreamRate::eighth:        return (int64_t) (quarter * 0.5);
        case StreamRate::sixteenth:     return (int64_t) (quarter * 0.25);
        case StreamRate::tripletEighth: return (int64_t) (quarter / 3.0);
    }
    return (int64_t) (quarter * 0.5);
}

void PatternEngine::generateWindow (const StreamContext& ctx,
                                    int64_t windowStart, int64_t windowEnd,
                                    ScheduledNoteList<64>& out)
{
    switch (ctx.profile.mode)
    {
        case PerformanceMode::pulse:   generatePulse (ctx, windowStart, windowEnd, out); break;
        case PerformanceMode::arp:     generateArp (ctx, windowStart, windowEnd, out); break;
        case PerformanceMode::pattern: generatePattern (ctx, windowStart, windowEnd, out); break;
        default: break;
    }
}

//==============================================================================
// PULSE: the whole chord re-strikes on the grid, accent-shaped.

void PatternEngine::generatePulse (const StreamContext& ctx,
                                   int64_t windowStart, int64_t windowEnd,
                                   ScheduledNoteList<64>& out)
{
    const auto& p = ctx.profile.pulse;
    const auto& v = ctx.realization.voicing;
    if (v.count == 0)
        return;

    const int64_t sub = rateToSamples (p.rate, ctx.sampleRate, ctx.bpm);
    if (sub <= 0)
        return;

    const int64_t start = ctx.streamStartSample;
    const int64_t gate = std::max<int64_t> (1, (int64_t) ((double) sub * (double) p.gateRatio));
    Rng rng { (ctx.seed ^ 0x51ABu) | 1u };

    const int64_t k0 = std::max<int64_t> (0, (windowStart - start) / sub);
    for (int64_t k = k0; start + k * sub < windowEnd && out.count < 60; ++k)
    {
        // Swing shifts every second grid point.
        const int64_t swing = (p.swing > 0.0f && (k % 2 == 1))
            ? (int64_t) (p.swing * (float) sub) : 0;
        const int64_t on = start + k * sub + swing;

        if (on < windowStart)
            continue;

        const bool accent = p.accentEvery > 0 && (k % p.accentEvery == 0);
        const int base = ctx.profile.velocityBaseline;

        for (int i = 0; i < v.count; ++i)
        {
            const auto& voice = v.voices[(size_t) i];
            if (p.bassHold && voice.role == VoiceRole::bass)
                continue; // bass holds through (scheduled once at stream start)

            const int vel = (int) ((float) base * (accent ? 1.0f : 0.72f))
                          + roleWeight (voice.role)
                          + (int) (rng.nextBipolar() * 3.0f);
            out.add (makeNote (voice.pitch.value, vel, on - start, gate, voice.role));
        }
    }
}

//==============================================================================
// ARP: cycles the chord tones (octave-expanded) one note per grid point.

void PatternEngine::generateArp (const StreamContext& ctx,
                                 int64_t windowStart, int64_t windowEnd,
                                 ScheduledNoteList<64>& out)
{
    const auto& a = ctx.profile.arp;
    const auto& v = ctx.realization.voicing;
    if (v.count == 0)
        return;

    const int64_t sub = rateToSamples (a.rate, ctx.sampleRate, ctx.bpm);
    if (sub <= 0)
        return;

    // Ordered tone list (ascending), octave-expanded.
    std::array<Voice, 24> tones {};
    int toneCount = 0;
    for (int oct = 0; oct < std::clamp (a.octaves, 1, 2); ++oct)
        for (int i = 0; i < v.count; ++i)
        {
            Voice t = v.voices[(size_t) i];
            t.pitch.value += oct * 12;
            tones[(size_t) toneCount++] = t;
        }

    const int64_t start = ctx.streamStartSample;
    const int64_t gate = std::max<int64_t> (1, (int64_t) ((double) sub * (double) a.gateRatio));
    Rng rng { (ctx.seed ^ 0xA9Fu) | 1u };

    // Sequence per direction (index sequence into tones).
    auto toneIndexAt = [&] (int64_t k) -> int
    {
        if (a.direction == ArpDirection::up)
            return (int) (k % toneCount);
        if (a.direction == ArpDirection::down)
            return toneCount - 1 - (int) (k % toneCount);
        // upDown: bounce without repeating the endpoints.
        const int period = 2 * toneCount - 2;
        if (period <= 0)
            return 0;
        const int x = (int) (k % period);
        return x < toneCount ? x : period - x;
    };

    const int64_t k0 = std::max<int64_t> (0, (windowStart - start) / sub);
    for (int64_t k = k0; start + k * sub < windowEnd && out.count < 60; ++k)
    {
        const int64_t on = start + k * sub;
        if (on < windowStart)
            continue;

        const auto& tone = tones[(size_t) toneIndexAt (k)];
        if (tone.pitch.value > 127)
            continue;

        // A held top voice is scheduled once at stream start; skip it here.
        if (a.topHold && tone.role == VoiceRole::top)
            continue;

        const int vel = ctx.profile.velocityBaseline
                      + roleWeight (tone.role)
                      + (int) (rng.nextBipolar() * 3.0f);
        out.add (makeNote (tone.pitch.value, vel, on - start, gate, tone.role));
    }
}

//==============================================================================
// PATTERN: curated one-bar loops on an 8-slot (eighth-note) grid.

void PatternEngine::generatePattern (const StreamContext& ctx,
                                     int64_t windowStart, int64_t windowEnd,
                                     ScheduledNoteList<64>& out)
{
    const auto kind = ctx.profile.pattern;
    const auto& v = ctx.realization.voicing;
    if (v.count == 0)
        return;

    const int64_t sub = rateToSamples (StreamRate::eighth, ctx.sampleRate, ctx.bpm);
    if (sub <= 0)
        return;

    const int64_t start = ctx.streamStartSample;
    Rng rng { (ctx.seed ^ 0xB0CEu) | 1u };

    auto bassPitch = [&] { return v.voices[0].pitch.value; };
    auto addUppers = [&] (int64_t on, int64_t len, float velScale)
    {
        for (int i = 1; i < v.count; ++i)
        {
            const auto& voice = v.voices[(size_t) i];
            const int vel = (int) (ctx.profile.velocityBaseline * velScale)
                          + roleWeight (voice.role)
                          + (int) (rng.nextBipolar() * 3.0f);
            out.add (makeNote (voice.pitch.value, vel, on - start, len, voice.role));
        }
    };
    auto addBass = [&] (int64_t on, int64_t len, float velScale)
    {
        const int vel = (int) (ctx.profile.velocityBaseline * velScale) - 4
                      + (int) (rng.nextBipolar() * 2.0f);
        out.add (makeNote (bassPitch(), vel, on - start, len, VoiceRole::bass));
    };

    const int64_t bar = sub * 8;
    (void) bar;
    auto slotAt = [&] (int64_t t) { return (t - start) / sub; };

    for (int64_t k = std::max<int64_t> (0, slotAt (windowStart));
         start + k * sub < windowEnd && out.count < 60; ++k)
    {
        const int64_t on = start + k * sub;
        if (on < windowStart)
            continue;
        const int slot = (int) ((k % 8 + 8) % 8);

        switch (kind)
        {
            case PatternKind::bounce:
            {
                // bass on 1 & the-and-of-3; uppers answer on 2 & 4-and.
                if (slot == 0) addBass (on, sub, 1.0f);
                if (slot == 3) addUppers (on, sub, 0.85f);
                if (slot == 4) addBass (on, sub, 0.8f);
                if (slot == 6) addUppers (on, sub, 0.95f);
                break;
            }
            case PatternKind::float_:
            {
                // Slow ascending swell: one tone every 2 slots, top holds.
                if ((k % 2) == 0)
                {
                    const int idx = (int) ((k / 2) % v.count);
                    const auto& voice = v.voices[(size_t) idx];
                    if (voice.role != VoiceRole::top)
                    {
                        const int vel = (int) (ctx.profile.velocityBaseline * 0.8f)
                                      + roleWeight (voice.role)
                                      + (int) (rng.nextBipolar() * 2.0f);
                        out.add (makeNote (voice.pitch.value, vel, on - start, sub * 2, voice.role));
                    }
                }
                break;
            }
            case PatternKind::stab:
            {
                // Syncopated full-chord hits: slots 1, 3, 6.
                if (slot == 1 || slot == 3 || slot == 6)
                {
                    const float velScale = slot == 3 ? 1.0f : 0.85f;
                    for (int i = 0; i < v.count; ++i)
                    {
                        const auto& voice = v.voices[(size_t) i];
                        const int vel = (int) (ctx.profile.velocityBaseline * velScale)
                                      + roleWeight (voice.role)
                                      + (int) (rng.nextBipolar() * 3.0f);
                        out.add (makeNote (voice.pitch.value, vel, on - start,
                                           (int64_t) (sub * 1.5), voice.role));
                    }
                }
                break;
            }
        }
    }
}

} // namespace morph
