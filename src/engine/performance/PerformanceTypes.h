#pragma once

#include <cstdint>

namespace morph
{

/** HOW a chord is played — never WHAT chord is generated. */
enum class PerformanceMode : uint8_t
{
    together = 0,
    strumUp,
    strumDown
    // V2+: pulse, pattern, arp
};

enum class TogetherKind : uint8_t
{
    tight = 0,
    soft,
    human,
    wide
};

enum class StrumCurve : uint8_t
{
    linear = 0,
    easeIn,
    easeOut,
    human
};

enum class StrumVelocityShape : uint8_t
{
    flat = 0,
    rise,   // low → high crescendo
    fall
};

enum class StrumDirection : uint8_t
{
    up = 0,
    down
};

/** How the bass voice participates in a strum. */
enum class BassStrumPolicy : uint8_t
{
    withStrum = 0,
    anchorFirst,
    anchorSimultaneous,
    delayed,
    excluded
};

/** Protects the melodic identity of the top voice. */
enum class TopVoicePerformancePolicy : uint8_t
{
    normal = 0,
    arriveLast,
    arriveFirst,
    accent,
    hold
};

struct TogetherProfile
{
    TogetherKind kind = TogetherKind::tight;

    /** Max absolute onset offset in milliseconds for the profile. */
    float maxOffsetMs() const
    {
        switch (kind)
        {
            case TogetherKind::tight: return 0.8f;
            case TogetherKind::soft:  return 2.0f;
            case TogetherKind::human: return 3.5f;
            case TogetherKind::wide:  return 6.0f;
        }
        return 0.8f;
    }
};

struct StrumProfile
{
    StrumDirection direction = StrumDirection::up;
    float spreadMs = 42.0f;
    StrumCurve curve = StrumCurve::human;
    StrumVelocityShape velocityShape = StrumVelocityShape::rise;
    BassStrumPolicy bassPolicy = BassStrumPolicy::withStrum;
    TopVoicePerformancePolicy topVoicePolicy = TopVoicePerformancePolicy::normal;
    float randomTimingAmount = 0.5f;   // 0..1, musician-like bounded variation
    float randomVelocityAmount = 0.25f;
};

/** Full performance description for one realization. */
struct PerformanceProfile
{
    PerformanceMode mode = PerformanceMode::together;
    TogetherProfile together;
    StrumProfile strum;

    int velocityBaseline = 96;   // OUTPUT knob maps here
    float humanizeTiming = 0.0f; // TEXTURE knob feeds these (0..1)
    float humanizeVelocity = 0.0f;
};

} // namespace morph
