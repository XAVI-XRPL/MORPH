#pragma once

#include "../harmony/HarmonyEngine.h"
#include "../voicing/VoicingEngine.h"

namespace morph
{

/**
 * Realizer: stateless Harmony → Voicing pipeline for generation/export
 * contexts (no EngineHost needed). Deterministic.
 */
struct Realizer
{
    KeyContext key;
    StyleProfile style = StyleProfile::modernRnB();
    float color = 0.5f;
    float openness = 0.4f;
    float motion = 0.5f;

    ChordRealization realize (ScaleDegree degree, VoiceLeadingMemory& mem, int slotIndex) const
    {
        HarmonyEngine harmony;
        VoicingEngine voicing;

        const auto candidate = harmony.chordForDegree (key, degree, style, color);

        VoicingContext ctx;
        if (mem.valid)
        {
            ctx.previous = &mem.voicing;
            ctx.previousBass = MidiPitch { mem.bass };
            ctx.hasPreviousBass = true;
            ctx.previousTop = MidiPitch { mem.top };
            ctx.hasPreviousTop = true;
            ctx.previousTopDirection = mem.topDirection;
        }
        ctx.slotIndex = slotIndex;
        ctx.motion = motion;
        ctx.openness = openness;
        ctx.tonalCenter = key.tonic;

        auto r = voicing.realize (candidate, key, style, ctx, 0);
        advanceVoiceLeadingMemory (mem, r);
        return r;
    }
};

} // namespace morph
