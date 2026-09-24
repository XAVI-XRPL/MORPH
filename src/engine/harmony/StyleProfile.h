#pragma once

#include "../theory/Chord.h"
#include "../performance/PerformanceTypes.h"
#include <array>

namespace morph
{

enum class StyleId
{
    modernRnB = 0,
    darkRnB,
    neoSoul,
    emotional,
    darkPop,
    trap,
    reggaeton // reserved (M7 — needs rhythmic engine)
};

inline constexpr int numImplementedStyles = 6; // all except reggaeton

/** One degree of a style's harmonic vocabulary (major-lattice roman). */
struct DegreeSpec
{
    int latticeDegree;      // 1..7
    int accidental;         // -1 flat, 0 natural
    ChordQuality quality;
    ChordExtensionSet extensions;
};

/**
 * StyleProfile v2 (M6): the vocabulary AND the grammar of a style.
 *
 * - degrees: degree → chord identity (natural-minor lattice)
 * - transitions: [from-1][to-1] likelihood weights 0..1 (functional grammar)
 * - tensionTargets: desired tension per progression slot (arc shape)
 * - cadenceWeight: how strongly slot 4 → slot 1 resolution matters
 */
struct StyleProfile
{
    StyleId id = StyleId::modernRnB;
    const char* displayName = "Modern R&B";

    std::array<DegreeSpec, 7> degrees {};
    std::array<std::array<float, 7>, 7> transitions {};
    std::array<float, 4> tensionTargets { 0.15f, 0.45f, 0.55f, 0.8f };
    float cadenceWeight = 1.0f;

    // Recommended performance behavior (explicit user selection wins, §56).
    BassStrumPolicy recommendedBassPolicy = BassStrumPolicy::anchorFirst;
    float recommendedStrumSpreadMs = 42.0f;
    int preferredBassOctave = 2;
    bool preferNinthChords = true;
    float brightness = 0.5f; // default COLOR center for the style

    float transitionWeight (int fromDegree, int toDegree) const
    {
        return transitions[(size_t) (fromDegree - 1)][(size_t) (toDegree - 1)];
    }

    /** Degree tension: how much harmonic gravity a degree carries. */
    static float degreeTension (int degree);

    static StyleProfile get (StyleId id);
    static StyleProfile modernRnB();
};

} // namespace morph
