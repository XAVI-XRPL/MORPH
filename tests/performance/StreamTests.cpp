#include "../TestHarness.h"
#include "engine/performance/PatternEngine.h"
#include "engine/performance/PerformanceEngine.h"
#include "engine/harmony/HarmonyEngine.h"
#include "engine/voicing/VoicingEngine.h"

using namespace morph;

namespace
{
    constexpr double sr = 48000.0;
    constexpr double bpm = 120.0; // quarter = 24000 samples

    ChordRealization goldenCm9()
    {
        const auto key = KeyContext::cMinor();
        const auto style = StyleProfile::modernRnB();
        HarmonyEngine harmony;
        VoicingEngine voicing;
        const auto candidate = harmony.chordForDegree (key, ScaleDegree { 1 }, style, 0.5f);
        VoicingContext ctx;
        ctx.openness = 0.4f;
        return voicing.realize (candidate, key, style, ctx, 0);
    }

    StreamContext makeCtx (PerformanceMode mode, const ChordRealization& r)
    {
        StreamContext ctx;
        ctx.realization = r;
        ctx.profile.mode = mode;
        ctx.sampleRate = sr;
        ctx.bpm = bpm;
        ctx.seed = 99;
        ctx.streamStartSample = 0;
        return ctx;
    }
}

MORPH_TEST (stream, pulseEventsLandOnGrid)
{
    // §102 tempo sync: pulse attacks land exactly on the eighth-note grid.
    const auto r = goldenCm9();
    auto ctx = makeCtx (PerformanceMode::pulse, r);
    ctx.profile.pulse.rate = StreamRate::eighth;
    ctx.profile.pulse.swing = 0.0f;

    ScheduledNoteList<64> events;
    PatternEngine::generateWindow (ctx, 0, 48000, events); // 1 second

    const int64_t sub = PatternEngine::rateToSamples (StreamRate::eighth, sr, bpm); // 12000
    CHECK (sub == 12000);
    CHECK (events.count > 0);

    const int voices = r.voicing.count;
    for (int i = 0; i < events.count; ++i)
    {
        CHECK (events.notes[(size_t) i].noteOnSampleOffset % sub == 0);
        CHECK (events.notes[(size_t) i].noteOffSampleOffset
               > events.notes[(size_t) i].noteOnSampleOffset);
    }

    // Full chord per grid point: 1 s / 12000 = 4 grid points × voices.
    CHECK_EQ (events.count, 4 * voices);
}

MORPH_TEST (stream, pulseAccentPattern)
{
    const auto r = goldenCm9();
    auto ctx = makeCtx (PerformanceMode::pulse, r);
    ctx.profile.pulse.rate = StreamRate::eighth;
    ctx.profile.pulse.accentEvery = 2;
    ctx.profile.velocityBaseline = 100;

    ScheduledNoteList<64> events;
    PatternEngine::generateWindow (ctx, 0, 48000, events);

    // k=0 (accent) vs k=1 (non-accent): same voice, accent is louder.
    const int voices = r.voicing.count;
    for (int v = 0; v < voices; ++v)
        CHECK (events.notes[0].velocity > events.notes[(size_t) voices].velocity - 8);
}

MORPH_TEST (stream, arpCyclesAscending)
{
    const auto r = goldenCm9();
    auto ctx = makeCtx (PerformanceMode::arp, r);
    ctx.profile.arp.direction = ArpDirection::up;
    ctx.profile.arp.rate = StreamRate::sixteenth;

    ScheduledNoteList<64> events;
    PatternEngine::generateWindow (ctx, 0, 36000, events); // 6 sixteenths at 120 BPM

    const int voices = r.voicing.count;
    CHECK (events.count >= voices);

    for (int i = 0; i < voices; ++i)
        CHECK_EQ (events.notes[(size_t) i].pitch, r.voicing.voices[(size_t) i].pitch.value);

    // Grid: 16ths at 120 BPM = 6000 samples.
    for (int i = 1; i < events.count; ++i)
        CHECK (events.notes[(size_t) i].noteOnSampleOffset
               > events.notes[(size_t) (i - 1)].noteOnSampleOffset);
}

MORPH_TEST (stream, arpDownCyclesDescending)
{
    const auto r = goldenCm9();
    auto ctx = makeCtx (PerformanceMode::arp, r);
    ctx.profile.arp.direction = ArpDirection::down;
    ctx.profile.arp.rate = StreamRate::sixteenth;

    ScheduledNoteList<64> events;
    PatternEngine::generateWindow (ctx, 0, 36000, events);

    const int voices = r.voicing.count;
    CHECK (events.count >= voices);
    for (int i = 0; i < voices; ++i)
        CHECK_EQ (events.notes[(size_t) i].pitch,
                  r.voicing.voices[(size_t) (voices - 1 - i)].pitch.value);
}

MORPH_TEST (stream, patternBounceSlots)
{
    const auto r = goldenCm9();
    auto ctx = makeCtx (PerformanceMode::pattern, r);
    ctx.profile.pattern = PatternKind::bounce;

    ScheduledNoteList<64> events;
    PatternEngine::generateWindow (ctx, 0, 96000, events); // one bar at 120 BPM

    const int64_t sub = 12000; // eighths at 120 BPM
    bool sawBass0 = false, sawUppers3 = false, sawBass4 = false, sawUppers6 = false;

    for (int i = 0; i < events.count; ++i)
    {
        const auto& n = events.notes[(size_t) i];
        const auto slot = n.noteOnSampleOffset / sub;
        if (slot == 0 && n.role == VoiceRole::bass) sawBass0 = true;
        if (slot == 3 && n.role != VoiceRole::bass) sawUppers3 = true;
        if (slot == 4 && n.role == VoiceRole::bass) sawBass4 = true;
        if (slot == 6 && n.role != VoiceRole::bass) sawUppers6 = true;
    }

    CHECK (sawBass0);
    CHECK (sawUppers3);
    CHECK (sawBass4);
    CHECK (sawUppers6);
}

MORPH_TEST (stream, deterministicWindows)
{
    // Window splits must not change output: refill == continuation.
    const auto r = goldenCm9();
    auto ctx = makeCtx (PerformanceMode::pulse, r);

    ScheduledNoteList<64> whole;
    PatternEngine::generateWindow (ctx, 0, 96000, whole);

    ScheduledNoteList<64> parts;
    PatternEngine::generateWindow (ctx, 0, 48000, parts);
    ScheduledNoteList<64> parts2;
    PatternEngine::generateWindow (ctx, 48000, 96000, parts2);
    for (int i = 0; i < parts2.count; ++i)
        parts.add (parts2.notes[(size_t) i]);

    CHECK_EQ (whole.count, parts.count);
    for (int i = 0; i < whole.count; ++i)
    {
        CHECK_EQ (whole.notes[(size_t) i].pitch, parts.notes[(size_t) i].pitch);
        CHECK_EQ (whole.notes[(size_t) i].noteOnSampleOffset,
                  parts.notes[(size_t) i].noteOnSampleOffset);
    }
}

MORPH_TEST (stream, strumDirections)
{
    // §54/M8: additional strum directions.
    const auto r = goldenCm9();

    auto strumWith = [&] (StrumDirection dir)
    {
        PerformanceProfile p;
        p.mode = PerformanceMode::strumUp;
        p.strum.direction = dir;
        p.strum.spreadMs = 100.0f;
        p.strum.curve = StrumCurve::linear;
        p.strum.velocityShape = StrumVelocityShape::flat;
        p.strum.bassPolicy = BassStrumPolicy::withStrum;
        p.strum.topVoicePolicy = TopVoicePerformancePolicy::normal;
        p.strum.randomTimingAmount = 0.0f;
        p.strum.randomVelocityAmount = 0.0f;
        return PerformanceEngine::schedule (r, p, sr, -1, 7);
    };

    // outsideIn: bass first, top second, then inward.
    {
        const auto notes = strumWith (StrumDirection::outsideIn);
        CHECK_EQ (notes.notes[0].pitch, 36); // bass
        CHECK_EQ (notes.notes[1].pitch, 63); // top
        CHECK_EQ (notes.notes[2].pitch, 43); // next-low
    }

    // insideOut: middle voice first, expanding outward.
    {
        const auto notes = strumWith (StrumDirection::insideOut);
        CHECK_EQ (notes.notes[0].pitch, 58); // middle (Bb3)
        CHECK_EQ (notes.count, 5);
    }

    // upDown: full ascent then descent (endpoints not repeated).
    {
        const auto notes = strumWith (StrumDirection::upDown);
        CHECK_EQ (notes.count, 2 * 5 - 2);
        CHECK_EQ (notes.notes[0].pitch, 36);
        CHECK_EQ (notes.notes[4].pitch, 63); // peak
        CHECK_EQ (notes.notes[(size_t) (notes.count - 1)].pitch, 43); // turns home
        for (int i = 1; i < notes.count; ++i)
            CHECK (notes.notes[(size_t) i].noteOnSampleOffset
                   >= notes.notes[(size_t) (i - 1)].noteOnSampleOffset);
    }

    // randomControlled: deterministic permutation of the same pitches.
    {
        const auto a = strumWith (StrumDirection::randomControlled);
        const auto b = strumWith (StrumDirection::randomControlled);
        CHECK_EQ (a.count, 5);
        for (int i = 0; i < a.count; ++i)
        {
            CHECK_EQ (a.notes[(size_t) i].pitch, b.notes[(size_t) i].pitch);
            CHECK (r.voicing.containsPitch (a.notes[(size_t) i].pitch));
        }
    }
}
