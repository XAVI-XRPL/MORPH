#include "QualityMetrics.h"
#include <cstdlib>

namespace morph
{

namespace
{
    float clamp01 (float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

    int extensionColorCount (const DegreeSpec& spec)
    {
        int n = 0;
        for (auto e : { extNinth, extFlat9, extSharp9, extEleventh, extSharp11, extFlat13, ext13th })
            if (hasExtension (spec.extensions, e))
                ++n;
        return n;
    }

    float blendTotal (const QualityScores& s)
    {
        return 0.22f * s.functionalCoherence
             + 0.14f * s.tensionArc
             + 0.14f * s.cadence
             + 0.12f * s.loopStrength
             + 0.12f * s.voiceLeadingQuality
             + 0.08f * s.memorability
             + 0.06f * s.vocalSpace
             + 0.08f * s.hookStrength
             + 0.04f * s.surprise;
    }
}

QualityScores QualityMetrics::evaluate (const std::array<int, 4>& degrees,
                                        const StyleProfile& style)
{
    QualityScores s;

    // Functional coherence: mean transition weight across the loop.
    float tw = 0.0f;
    int zeroWeightCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        const float w = style.transitionWeight (degrees[(size_t) i], degrees[(size_t) ((i + 1) % 4)]);
        tw += w;
        if (w < 0.05f)
            ++zeroWeightCount;
    }
    s.functionalCoherence = clamp01 (tw / 4.0f * 1.25f); // weights hover ~0.6-0.8 for good loops

    // Tension arc match.
    float tensionErr = 0.0f;
    for (int i = 0; i < 4; ++i)
        tensionErr += std::abs (StyleProfile::degreeTension (degrees[(size_t) i])
                                - style.tensionTargets[(size_t) i]);
    s.tensionArc = clamp01 (1.0f - tensionErr / 4.0f * 2.2f);

    // Cadence: slot 4 → slot 1 pull, scaled by style's cadence weight.
    s.cadence = clamp01 (style.transitionWeight (degrees[3], degrees[0]) * style.cadenceWeight * 1.1f);

    // Loop strength (sequence-only estimate; plan version refines with bass).
    s.loopStrength = clamp01 (0.65f * style.transitionWeight (degrees[3], degrees[0]) + 0.2f);

    // Memorability: repetition structure (degree returns, pairs).
    int repeats = 0;
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j)
            if (degrees[(size_t) i] == degrees[(size_t) j])
                ++repeats;
    s.memorability = clamp01 (0.45f + 0.25f * (float) repeats);

    // Vocal space: sparser middle slots leave room for a topline (§70).
    const float midColor = 0.5f * (float) (extensionColorCount (style.degrees[(size_t) (degrees[1] - 1)])
                                         + extensionColorCount (style.degrees[(size_t) (degrees[2] - 1)]));
    s.vocalSpace = clamp01 (1.0f - midColor * 0.25f);

    // Surprise: one or two non-obvious but valid moves; zero-weight is bad.
    int surprising = 0;
    for (int i = 0; i < 4; ++i)
    {
        const float w = style.transitionWeight (degrees[(size_t) i], degrees[(size_t) ((i + 1) % 4)]);
        if (w >= 0.3f && w < 0.6f)
            ++surprising;
    }
    s.surprise = clamp01 (0.15f * (float) surprising) - (zeroWeightCount > 0 ? 0.3f : 0.0f);
    s.surprise = clamp01 (s.surprise);

    s.styleAuthenticity = 1.0f; // generator only uses style vocabulary
    s.voiceLeadingQuality = 0.5f; // neutral without a plan
    s.hookStrength = 0.5f;

    s.total = blendTotal (s);
    return s;
}

QualityScores QualityMetrics::evaluateWithPlan (const std::array<int, 4>& degrees,
                                                const StyleProfile& style,
                                                const std::array<ChordRealization, 4>& plan)
{
    auto s = evaluate (degrees, style);

    // Voice-leading quality: mean nearest-neighbor movement per transition.
    float totalMove = 0.0f;
    int moves = 0;
    float topMoves = 0.0f;
    int topStepwise = 0;

    for (int i = 0; i < 4; ++i)
    {
        const auto& a = plan[(size_t) i].voicing;
        const auto& b = plan[(size_t) ((i + 1) % 4)].voicing;

        for (int v = 0; v < a.count; ++v)
        {
            int dmin = 24;
            for (int u = 0; u < b.count; ++u)
                dmin = std::min (dmin, std::abs (a.voices[(size_t) v].pitch.value
                                               - b.voices[(size_t) u].pitch.value));
            totalMove += (float) std::min (dmin, 12);
            ++moves;
        }

        const int topLeap = std::abs (plan[(size_t) ((i + 1) % 4)].topPitch.value
                                      - plan[(size_t) i].topPitch.value);
        topMoves += (float) topLeap;
        if (topLeap <= 2)
            ++topStepwise;
    }

    s.voiceLeadingQuality = clamp01 (1.0f - (totalMove / (float) moves) / 10.0f);
    s.hookStrength = clamp01 ((float) topStepwise / 4.0f + 0.2f);

    // Loop strength refined: bass returns home by step.
    const int bassReturn = std::abs (plan[3].bassPitch.value - plan[0].bassPitch.value);
    const float bassScore = bassReturn == 0 ? 0.9f : bassReturn <= 4 ? 1.0f : bassReturn <= 7 ? 0.6f : 0.3f;
    s.loopStrength = clamp01 (0.6f * style.transitionWeight (degrees[3], degrees[0])
                              + 0.4f * bassScore);

    // Memorability refined: top-line pitch-class repetition.
    int topRepeats = 0;
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j)
            if (pitchClassOf (plan[(size_t) i].topPitch) == pitchClassOf (plan[(size_t) j].topPitch))
                ++topRepeats;
    s.memorability = clamp01 (s.memorability + 0.12f * (float) topRepeats);

    s.total = blendTotal (s);
    return s;
}

} // namespace morph
