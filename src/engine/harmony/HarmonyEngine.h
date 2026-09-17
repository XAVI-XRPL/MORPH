#pragma once

#include "../theory/KeyContext.h"
#include "../theory/Chord.h"
#include "StyleProfile.h"

namespace morph
{

/**
 * HarmonyEngine: scale degree + key + style → chord identity.
 * Reasons about function and style vocabulary; never random.
 * V1: Modern R&B natural-minor functional map with COLOR-controlled
 * extension density.
 */
class HarmonyEngine
{
public:
    /**
     * colorAmount 0..1 (COLOR knob):
     *  < 0.33  darker  — seventh chords (extensions reduced)
     *  0.33..0.66      — canonical ninth vocabulary (Cm9, bVImaj9, ...)
     *  > 0.66  brighter — adds eleventh color on minor functions
     */
    ChordCandidate chordForDegree (const KeyContext& key,
                                   ScaleDegree degree,
                                   const StyleProfile& style,
                                   float colorAmount) const;
};

} // namespace morph
