#pragma once

#include "Voicing.h"
#include "../theory/KeyContext.h"
#include "../harmony/StyleProfile.h"
#include "../bass/BassEngine.h"
#include "../topline/TopVoiceEngine.h"

namespace morph
{

/** Voice-leading memory threaded across realizations (bass/top/direction +
    the previous voicing for movement scoring). */
struct VoiceLeadingMemory
{
    bool valid = false;
    Voicing voicing {};
    int bass = -1;
    int top = -1;
    int topDirection = 0; // -1 falling, 0 static, +1 rising
};

inline void advanceVoiceLeadingMemory (VoiceLeadingMemory& mem, const ChordRealization& r)
{
    const int prevTop = mem.top;
    mem.valid = true;
    mem.voicing = r.voicing;
    mem.bass = r.bassPitch.value;
    mem.top = r.topPitch.value;
    mem.topDirection = prevTop < 0 ? 0
                    : r.topPitch.value > prevTop ? 1
                    : r.topPitch.value < prevTop ? -1 : 0;
}

/** Sequence context for voice leading (previous chord memory). */
struct VoicingContext
{
    const Voicing* previous = nullptr;
    MidiPitch previousBass { -1 };
    bool hasPreviousBass = false;
    MidiPitch previousTop { -1 };
    bool hasPreviousTop = false;
    int previousTopDirection = 0;   // -1 falling, 0 static, +1 rising
    int slotIndex = 0;              // progression position (bass bounce)
    float motion = 0.5f;            // MOTION knob
    float openness = 0.4f;          // SPACE knob
    PitchClass tonalCenter { 0 };
};

/**
 * VoicingEngine (§48): chord identity → physical voicing.
 *
 * M3: candidate generation + scoring. Bass options come from BassEngine,
 * top options from TopVoiceEngine, inner voices fill register bands.
 * Candidates are scored for voice movement, common tones, leaps, crossing,
 * register, spacing, low-end cleanliness, bass quality, top-line quality.
 *
 * With no previous context, the canonical register-band recipe is used
 * (golden Cm9 = C2 G2 Bb3 D4 Eb4 preserved).
 *
 * Stateless: safe to call from any thread with independent arguments.
 */
class VoicingEngine
{
public:
    ChordRealization realize (const ChordCandidate& candidate,
                              const KeyContext& key,
                              const StyleProfile& style,
                              const VoicingContext& context,
                              int variation = 0) const;

    /** Deterministic voicing sibling transform (MORPH action, pre-M5). */
    static void applyVariation (Voicing& v, int variation);

private:
    BassEngine bassEngine;
    TopVoiceEngine topVoiceEngine;
};

} // namespace morph
