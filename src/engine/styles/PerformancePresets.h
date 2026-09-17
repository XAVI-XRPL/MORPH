#pragma once

#include "../performance/PerformanceTypes.h"

namespace morph
{

/** Curated performance presets (§58) — quality over quantity. */
struct PerformancePreset
{
    const char* name;
    PerformanceMode mode;
    TogetherKind togetherKind;
    float spreadMs;
    StrumCurve curve;
    StrumVelocityShape velocityShape;
    BassStrumPolicy bassPolicy;
    TopVoicePerformancePolicy topPolicy;
};

inline constexpr PerformancePreset performancePresets[] =
{
    // TOGETHER
    { "TIGHT",     PerformanceMode::together, TogetherKind::tight,  42.0f, StrumCurve::linear,  StrumVelocityShape::flat, BassStrumPolicy::withStrum,   TopVoicePerformancePolicy::normal },
    { "SOFT",      PerformanceMode::together, TogetherKind::soft,   42.0f, StrumCurve::linear,  StrumVelocityShape::flat, BassStrumPolicy::withStrum,   TopVoicePerformancePolicy::normal },
    { "HUMAN",     PerformanceMode::together, TogetherKind::human,  42.0f, StrumCurve::linear,  StrumVelocityShape::flat, BassStrumPolicy::withStrum,   TopVoicePerformancePolicy::normal },

    // STRUM
    { "SOFT UP",   PerformanceMode::strumUp,  TogetherKind::soft,   60.0f, StrumCurve::easeOut, StrumVelocityShape::rise, BassStrumPolicy::anchorFirst, TopVoicePerformancePolicy::normal },
    { "SOFT DOWN", PerformanceMode::strumDown,TogetherKind::soft,   60.0f, StrumCurve::easeOut, StrumVelocityShape::fall, BassStrumPolicy::anchorFirst, TopVoicePerformancePolicy::arriveLast },
    { "FAST UP",   PerformanceMode::strumUp,  TogetherKind::tight,  22.0f, StrumCurve::linear,  StrumVelocityShape::rise, BassStrumPolicy::withStrum,   TopVoicePerformancePolicy::normal },
    { "FAST DOWN", PerformanceMode::strumDown,TogetherKind::tight,  22.0f, StrumCurve::linear,  StrumVelocityShape::fall, BassStrumPolicy::withStrum,   TopVoicePerformancePolicy::normal },
    { "LAZY UP",   PerformanceMode::strumUp,  TogetherKind::human,  90.0f, StrumCurve::easeIn,  StrumVelocityShape::rise, BassStrumPolicy::anchorFirst, TopVoicePerformancePolicy::arriveLast },
    { "LAZY DOWN", PerformanceMode::strumDown,TogetherKind::human,  90.0f, StrumCurve::easeIn,  StrumVelocityShape::fall, BassStrumPolicy::anchorFirst, TopVoicePerformancePolicy::arriveLast },
    { "INTIMATE",  PerformanceMode::strumUp,  TogetherKind::soft,   55.0f, StrumCurve::human,   StrumVelocityShape::rise, BassStrumPolicy::anchorFirst, TopVoicePerformancePolicy::accent },
    { "WIDE",      PerformanceMode::strumDown,TogetherKind::wide,  110.0f, StrumCurve::easeOut, StrumVelocityShape::flat, BassStrumPolicy::withStrum,   TopVoicePerformancePolicy::hold },
};

inline constexpr int numPerformancePresets = (int) (sizeof (performancePresets) / sizeof (PerformancePreset));

} // namespace morph
