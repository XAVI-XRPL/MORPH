#include "../TestHarness.h"
#include "engine/harmony/HarmonyEngine.h"
#include "engine/voicing/VoicingEngine.h"
#include "engine/performance/PerformanceEngine.h"

using namespace morph;

namespace
{
    constexpr double sr = 48000.0;

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

    PerformanceProfile togetherProfile()
    {
        PerformanceProfile p;
        p.mode = PerformanceMode::together;
        p.together.kind = TogetherKind::tight;
        return p;
    }

    PerformanceProfile strumProfile (PerformanceMode mode, float spreadMs,
                                     StrumCurve curve = StrumCurve::linear)
    {
        PerformanceProfile p;
        p.mode = mode;
        p.strum.direction = mode == PerformanceMode::strumUp ? StrumDirection::up
                                                             : StrumDirection::down;
        p.strum.spreadMs = spreadMs;
        p.strum.curve = curve;
        p.strum.velocityShape = StrumVelocityShape::flat;
        p.strum.bassPolicy = BassStrumPolicy::withStrum;
        p.strum.topVoicePolicy = TopVoicePerformancePolicy::normal;
        p.strum.randomTimingAmount = 0.0f;
        p.strum.randomVelocityAmount = 0.0f;
        return p;
    }
}

MORPH_TEST (performance, togetherAttacksWithinTolerance)
{
    const auto r = goldenCm9();
    const auto notes = PerformanceEngine::schedule (r, togetherProfile(), sr, -1, 42);

    CHECK_EQ (notes.count, r.voicing.count);

    int64_t earliest = INT64_MAX, latest = 0;
    for (int i = 0; i < notes.count; ++i)
    {
        earliest = std::min (earliest, notes.notes[(size_t) i].noteOnSampleOffset);
        latest = std::max (latest, notes.notes[(size_t) i].noteOnSampleOffset);
    }

    const float windowMs = (float) (latest - earliest) / (float) sr * 1000.0f;
    CHECK (windowMs <= PerformanceEngine::togetherToleranceMs);
}

MORPH_TEST (performance, strumUpAscendingOrder)
{
    const auto r = goldenCm9();
    const auto notes = PerformanceEngine::schedule (r, strumProfile (PerformanceMode::strumUp, 100.0f), sr, -1, 7);

    CHECK_EQ (notes.count, 5);

    // Spec §105: STRUM UP attacks C2 → G2 → Bb3 → D4 → Eb4 in time order.
    for (int i = 1; i < notes.count; ++i)
    {
        CHECK (notes.notes[(size_t) i].pitch > notes.notes[(size_t) (i - 1)].pitch);
        CHECK (notes.notes[(size_t) i].noteOnSampleOffset >= notes.notes[(size_t) (i - 1)].noteOnSampleOffset);
    }
}

MORPH_TEST (performance, strumDownDescendingOrder)
{
    const auto r = goldenCm9();
    const auto notes = PerformanceEngine::schedule (r, strumProfile (PerformanceMode::strumDown, 100.0f), sr, -1, 7);

    CHECK_EQ (notes.count, 5);

    // Spec §105: STRUM DOWN attacks Eb4 → D4 → Bb3 → G2 → C2.
    for (int i = 1; i < notes.count; ++i)
    {
        CHECK (notes.notes[(size_t) i].pitch < notes.notes[(size_t) (i - 1)].pitch);
        CHECK (notes.notes[(size_t) i].noteOnSampleOffset >= notes.notes[(size_t) (i - 1)].noteOnSampleOffset);
    }
}

MORPH_TEST (performance, strumSpreadMatchesConfiguration)
{
    const auto r = goldenCm9();
    const float spreadMs = 100.0f;
    const auto notes = PerformanceEngine::schedule (r, strumProfile (PerformanceMode::strumUp, spreadMs), sr, -1, 7);

    int64_t earliest = INT64_MAX, latest = 0;
    for (int i = 0; i < notes.count; ++i)
    {
        earliest = std::min (earliest, notes.notes[(size_t) i].noteOnSampleOffset);
        latest = std::max (latest, notes.notes[(size_t) i].noteOnSampleOffset);
    }

    // Spec §55: spread = time from first attack to final attack (5 voices, 100 ms).
    const float actualMs = (float) (latest - earliest) / (float) sr * 1000.0f;
    CHECK (std::abs (actualMs - spreadMs) < 0.2f);
}

MORPH_TEST (performance, strumCurves)
{
    const auto r = goldenCm9();

    const auto linear = PerformanceEngine::schedule (r, strumProfile (PerformanceMode::strumUp, 100.0f, StrumCurve::linear), sr, -1, 7);
    const auto easeIn = PerformanceEngine::schedule (r, strumProfile (PerformanceMode::strumUp, 100.0f, StrumCurve::easeIn), sr, -1, 7);

    // Ease-in starts slow: the middle voice attacks EARLIER than linear's
    // (x=0.5 → eased 0.25), while the final attack still lands at full spread.
    const int mid = 2;
    CHECK (easeIn.notes[(size_t) mid].noteOnSampleOffset < linear.notes[(size_t) mid].noteOnSampleOffset);

    // Both still span the same total spread.
    CHECK_EQ (easeIn.notes[0].noteOnSampleOffset, 0);
    CHECK_EQ (easeIn.notes[(size_t) (easeIn.count - 1)].noteOnSampleOffset,
              linear.notes[(size_t) (linear.count - 1)].noteOnSampleOffset);
}

MORPH_TEST (performance, determinism)
{
    const auto r = goldenCm9();
    auto p = strumProfile (PerformanceMode::strumUp, 60.0f, StrumCurve::human);
    p.strum.randomTimingAmount = 1.0f;
    p.strum.randomVelocityAmount = 1.0f;
    p.humanizeVelocity = 1.0f;

    const auto a = PerformanceEngine::schedule (r, p, sr, -1, 1234);
    const auto b = PerformanceEngine::schedule (r, p, sr, -1, 1234);

    CHECK_EQ (a.count, b.count);
    for (int i = 0; i < a.count; ++i)
    {
        CHECK_EQ (a.notes[(size_t) i].noteOnSampleOffset, b.notes[(size_t) i].noteOnSampleOffset);
        CHECK_EQ (a.notes[(size_t) i].velocity, b.notes[(size_t) i].velocity);
    }
}

MORPH_TEST (performance, bassAnchorFirstPolicy)
{
    auto r = goldenCm9();
    auto p = strumProfile (PerformanceMode::strumDown, 80.0f);

    // Default golden: bass last in strum down (withStrum).
    const auto golden = PerformanceEngine::schedule (r, p, sr, -1, 7);
    CHECK_EQ (golden.notes[(size_t) (golden.count - 1)].pitch, 36);

    // AnchorFirst (Modern R&B recommendation): bass attacks first even in strum down.
    p.strum.bassPolicy = BassStrumPolicy::anchorFirst;
    const auto anchored = PerformanceEngine::schedule (r, p, sr, -1, 7);
    CHECK_EQ (anchored.notes[0].pitch, 36);                  // C2 first
    CHECK (anchored.notes[1].pitch > anchored.notes[2].pitch); // rest descends
}

MORPH_TEST (performance, topVoiceArriveLastPolicy)
{
    const auto r = goldenCm9();
    auto p = strumProfile (PerformanceMode::strumDown, 80.0f);
    p.strum.topVoicePolicy = TopVoicePerformancePolicy::arriveLast;

    const auto notes = PerformanceEngine::schedule (r, p, sr, -1, 7);
    CHECK_EQ (notes.notes[(size_t) (notes.count - 1)].pitch, 63); // Eb4 arrives last
}

MORPH_TEST (performance, velocityShapeRise)
{
    const auto r = goldenCm9();
    auto p = strumProfile (PerformanceMode::strumUp, 80.0f);
    p.strum.velocityShape = StrumVelocityShape::rise;
    p.velocityBaseline = 100;

    const auto notes = PerformanceEngine::schedule (r, p, sr, -1, 7);

    // Rise: velocity increases along the strum (ignoring small role weights).
    const int first = notes.notes[0].velocity;
    const int last = notes.notes[(size_t) (notes.count - 1)].velocity;
    CHECK (last > first + 10);
}
