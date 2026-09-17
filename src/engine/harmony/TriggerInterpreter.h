#pragma once

#include "../theory/KeyContext.h"

namespace morph
{

enum class TriggerMode : uint8_t
{
    scaleDegree = 0, // default: input note = scale degree
    root,
    smart
    // V1 implements SCALE_DEGREE.
};

enum class ChromaticInputPolicy : uint8_t
{
    styleColor = 0 // default: chromatic input maps toward valid function
};

/**
 * Interprets one input MIDI note against the key context.
 * SCALE_DEGREE mode: the note's diatonic degree selects the harmonic function.
 * Chromatic input snaps to the nearest scale degree (ties snap downward),
 * never producing arbitrary chromatic chords.
 */
struct TriggerResult
{
    ScaleDegree degree { 1 };
    bool wasChromatic = false;
};

class TriggerInterpreter
{
public:
    TriggerMode mode = TriggerMode::scaleDegree;

    TriggerResult interpret (MidiPitch input, const KeyContext& key) const;
};

} // namespace morph
