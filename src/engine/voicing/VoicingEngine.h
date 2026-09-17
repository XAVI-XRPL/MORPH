#pragma once

#include "Voicing.h"
#include "../theory/KeyContext.h"
#include "../harmony/StyleProfile.h"

namespace morph
{

/**
 * VoicingEngine: chord identity → physical voicing.
 * Chord != voicing. Deterministic register-band recipe with
 * top-voice continuity against the previous voicing.
 *
 * Register plan (golden family, bass octave 2):
 *   bass        root            [36..47]
 *   low inner   5th (or b5)     bass + 7
 *   mid inner   7th             octave 3 (48+pc)
 *   high inner  9th / 11th      octave 4 (60+pc)
 *   top         3rd / 4th       octave 4, above 9th
 *
 * Low-end cleanliness: single bass below MIDI 48; inner voices stay
 * at or above bass + 5 semitones.
 */
class VoicingEngine
{
public:
    /**
     * @param openness  SPACE knob 0..1 (close → open).
     * @param variation Deterministic voicing-sibling selector (MORPH action;
     *                  0 = canonical). Same chord identity, different shape.
     * Stateless: safe to call from any thread with independent arguments.
     */
    ChordRealization realize (const ChordCandidate& candidate,
                              const KeyContext& key,
                              const StyleProfile& style,
                              const Voicing* previous,
                              float openness,
                              int variation = 0) const;
};

} // namespace morph
