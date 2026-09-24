#pragma once

#include <cstdint>

namespace morph
{

/** HOW a chord is played — never WHAT chord is generated.
    One-shot modes: together/strum. Stream modes (M8): pulse/pattern/arp —
    the performance continues rhythmically while the trigger is held. */
enum class PerformanceMode : uint8_t
{
    together = 0,
    strumUp,
    strumDown,
    pulse,
    pattern,
    arp
};

inline constexpr int numPerformanceModes = 6;

inline bool isStreamMode (PerformanceMode m)
{
    return m == PerformanceMode::pulse || m == PerformanceMode::pattern
        || m == PerformanceMode::arp;
}

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
    down,
    upDown,
    downUp,
    outsideIn,
    insideOut,
    randomControlled
};

inline constexpr int numStrumDirections = 7;

/** Tempo-synced subdivision for stream modes. */
enum class StreamRate : uint8_t
{
    eighth = 0,     // 1/8 notes
    sixteenth,      // 1/16 notes
    tripletEighth   // 1/8 triplets
};

/** Curated PATTERN kinds (one-bar loops on an 8-slot grid, §102). */
enum class PatternKind : uint8_t
{
    bounce = 0, // bass anchors + chord stabs (offbeat answers)
    float_,   // slow ascending swell, top held
    stab      // syncopated full-chord hits
};

enum class ArpDirection : uint8_t
{
    up = 0,
    down,
    upDown
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

struct PulseProfile
{
    StreamRate rate = StreamRate::eighth;
    int accentEvery = 4;        // every Nth grid point is accented
    float gateRatio = 0.55f;    // note length = grid × gate
    float swing = 0.0f;         // 0..0.3 shifts every second grid point
    bool bassHold = false;      // bass sustains while uppers pulse
};

struct ArpProfile
{
    ArpDirection direction = ArpDirection::up;
    StreamRate rate = StreamRate::sixteenth;
    int octaves = 1;            // 1..2 tone expansion
    float gateRatio = 0.8f;
    bool topHold = false;       // top voice sustains over the arp
};

/** Full performance description for one realization. */
struct PerformanceProfile
{
    PerformanceMode mode = PerformanceMode::together;
    TogetherProfile together;
    StrumProfile strum;
    PulseProfile pulse;
    ArpProfile arp;
    PatternKind pattern = PatternKind::bounce;

    int velocityBaseline = 96;   // OUTPUT knob maps here
    float humanizeTiming = 0.0f; // TEXTURE knob feeds these (0..1)
    float humanizeVelocity = 0.0f;
};

} // namespace morph
