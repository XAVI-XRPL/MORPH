#pragma once

#include "../theory/Chord.h"
#include "../theory/KeyContext.h"

namespace morph
{

/** Bass behavior profiles (§50). Bass is melodic — never just every root. */
enum class BassMotionProfile
{
    root,   // chord root in the bass register (default foundation)
    flow,   // smoothest chord-tone choice toward the previous bass note
    pedal,  // hold the tonal center regardless of chord
    bounce  // alternate root / fifth per slot
};

struct BassContext
{
    MidiPitch previousBass { -1 };
    bool hasPrevious = false;
    PitchClass tonalCenter { 0 };
    int slotIndex = 0;    // progression position (for bounce)
    float motion = 0.5f;  // MOTION knob
    int profileOverride = -1; // >= 0 forces a BassMotionProfile (style-driven)
};

/**
 * BassEngine chooses the bass pitch for a chord. Register discipline:
 * bass lives in [36..47] (octave 2), transposing as needed (§49).
 */
class BassEngine
{
public:
    /** MOTION maps to profile: still → root, mid → flow, moving → bounce. */
    static BassMotionProfile profileForMotion (float motion);

    MidiPitch chooseBass (const ChordSymbol& chord,
                          const BassContext& context) const;
};

} // namespace morph
