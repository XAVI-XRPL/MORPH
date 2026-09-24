#include "StyleProfile.h"

namespace morph
{

namespace
{
    using T7 = std::array<std::array<float, 7>, 7>;

    // Base functional grammar for natural-minor urban styles.
    // Rows: from degree 1..7. Columns: to degree 1..7.
    T7 baseTransitions()
    {
        return T7 {{
            //  →1     →2     →3     →4     →5     →6     →7
            {{ 0.15f, 0.30f, 0.60f, 0.90f, 0.50f, 0.85f, 0.50f }}, // from i
            {{ 0.50f, 0.15f, 0.30f, 0.50f, 0.90f, 0.40f, 0.40f }}, // from iiø
            {{ 0.50f, 0.30f, 0.15f, 0.80f, 0.40f, 0.85f, 0.70f }}, // from bIII
            {{ 0.80f, 0.40f, 0.50f, 0.15f, 0.90f, 0.50f, 0.60f }}, // from iv
            {{ 0.95f, 0.20f, 0.40f, 0.35f, 0.10f, 0.60f, 0.40f }}, // from V
            {{ 0.50f, 0.30f, 0.60f, 0.75f, 0.50f, 0.15f, 0.80f }}, // from bVI
            {{ 0.75f, 0.30f, 0.70f, 0.60f, 0.55f, 0.55f, 0.15f }}, // from bVII
        }};
    }

    StyleProfile make (StyleId id, const char* name,
                       std::initializer_list<DegreeSpec> degrees)
    {
        StyleProfile p;
        p.id = id;
        p.displayName = name;
        p.transitions = baseTransitions();
        int i = 0;
        for (const auto& d : degrees)
            p.degrees[(size_t) i++] = d;
        return p;
    }
}

float StyleProfile::degreeTension (int degree)
{
    switch (degree)
    {
        case 1: return 0.10f;
        case 2: return 0.50f;
        case 3: return 0.30f;
        case 4: return 0.45f;
        case 5: return 0.80f;
        case 6: return 0.30f;
        case 7: return 0.55f;
        default: return 0.5f;
    }
}

StyleProfile StyleProfile::modernRnB()
{
    auto p = make (StyleId::modernRnB, "Modern R&B",
    {
        DegreeSpec { 1,  0, ChordQuality::minor,          extFlat7 | extNinth },  // i9
        DegreeSpec { 2,  0, ChordQuality::halfDiminished, extFlat7 },             // iiø7
        DegreeSpec { 3, -1, ChordQuality::major,          extMajor7 | extNinth }, // bIIImaj9
        DegreeSpec { 4,  0, ChordQuality::minor,          extFlat7 | extNinth },  // iv9
        DegreeSpec { 5,  0, ChordQuality::sus4,           extFlat7 },             // V7sus
        DegreeSpec { 6, -1, ChordQuality::major,          extMajor7 | extNinth }, // bVImaj9
        DegreeSpec { 7, -1, ChordQuality::dominant,       extNinth }              // bVII9
    });
    p.recommendedBassPolicy = BassStrumPolicy::anchorFirst;
    p.recommendedStrumSpreadMs = 42.0f;
    p.brightness = 0.5f;
    return p;
}

StyleProfile StyleProfile::get (StyleId id)
{
    switch (id)
    {
        case StyleId::darkRnB:
        {
            auto p = make (StyleId::darkRnB, "Dark R&B",
            {
                DegreeSpec { 1,  0, ChordQuality::minor,          extFlat7 | extNinth },
                DegreeSpec { 2,  0, ChordQuality::halfDiminished, extFlat7 },
                DegreeSpec { 3, -1, ChordQuality::major,          extMajor7 },
                DegreeSpec { 4,  0, ChordQuality::minor,          extFlat7 | extNinth },
                DegreeSpec { 5,  0, ChordQuality::minor,          extFlat7 }, // minor v: darker
                DegreeSpec { 6, -1, ChordQuality::major,          extMajor7 | extNinth },
                DegreeSpec { 7, -1, ChordQuality::dominant,       extNone }
            });
            p.tensionTargets = { 0.2f, 0.5f, 0.6f, 0.85f };
            p.brightness = 0.35f;
            p.recommendedStrumSpreadMs = 55.0f;
            p.transitions[4][0] = 0.85f; // v→i slightly weaker than V→i
            p.transitions[4][6] = 0.65f; // v→bVII dark walk
            return p;
        }

        case StyleId::neoSoul:
        {
            auto p = make (StyleId::neoSoul, "Neo-Soul",
            {
                DegreeSpec { 1,  0, ChordQuality::minor,          extFlat7 | extNinth | extEleventh }, // i11
                DegreeSpec { 2,  0, ChordQuality::halfDiminished, extFlat7 | extEleventh },
                DegreeSpec { 3, -1, ChordQuality::major,          extMajor7 | extNinth },
                DegreeSpec { 4,  0, ChordQuality::minor,          extFlat7 | extNinth | extEleventh }, // iv11
                DegreeSpec { 5,  0, ChordQuality::minor,          extFlat7 | extNinth }, // v9
                DegreeSpec { 6, -1, ChordQuality::major,          extMajor7 | extNinth },
                DegreeSpec { 7, -1, ChordQuality::dominant,       extNinth | ext13th }
            });
            p.tensionTargets = { 0.2f, 0.5f, 0.5f, 0.7f };
            p.brightness = 0.65f;
            p.transitions[1][4] = 0.95f; // iiø→v strong
            p.transitions[2][1] = 0.55f; // bIII→iiø passing
            p.transitions[6][4] = 0.75f; // bVII→v
            return p;
        }

        case StyleId::emotional:
        {
            auto p = make (StyleId::emotional, "Emotional",
            {
                DegreeSpec { 1,  0, ChordQuality::minor,          extFlat7 | extNinth },
                DegreeSpec { 2,  0, ChordQuality::halfDiminished, extFlat7 | extEleventh },
                DegreeSpec { 3, -1, ChordQuality::major,          extMajor7 | extNinth },
                DegreeSpec { 4,  0, ChordQuality::minor,          extFlat7 | extNinth | extEleventh },
                DegreeSpec { 5,  0, ChordQuality::sus4,           extFlat7 | extNinth }, // V9sus
                DegreeSpec { 6, -1, ChordQuality::major,          extMajor7 | extNinth },
                DegreeSpec { 7, -1, ChordQuality::dominant,       extNinth }
            });
            p.tensionTargets = { 0.15f, 0.5f, 0.6f, 0.85f };
            p.cadenceWeight = 1.3f; // strong resolution matters
            p.brightness = 0.55f;
            p.recommendedStrumSpreadMs = 60.0f;
            p.transitions[3][4] = 0.95f; // iv→V
            p.transitions[4][0] = 0.98f; // V→i payoff
            return p;
        }

        case StyleId::darkPop:
        {
            auto p = make (StyleId::darkPop, "Dark Pop",
            {
                DegreeSpec { 1,  0, ChordQuality::minor,          extFlat7 },
                DegreeSpec { 2,  0, ChordQuality::halfDiminished, extFlat7 },
                DegreeSpec { 3, -1, ChordQuality::major,          extMajor7 },
                DegreeSpec { 4,  0, ChordQuality::minor,          extFlat7 | extNinth },
                DegreeSpec { 5,  0, ChordQuality::sus4,           extFlat7 },
                DegreeSpec { 6, -1, ChordQuality::major,          extMajor7 | extNinth },
                DegreeSpec { 7, -1, ChordQuality::sus4,           extFlat7 } // bVII7sus
            });
            p.tensionTargets = { 0.2f, 0.4f, 0.55f, 0.75f };
            p.brightness = 0.4f;
            p.transitions[5][6] = 0.9f;  // bVI→bVII
            p.transitions[6][4] = 0.7f;  // bVII→v/i area
            p.transitions[3][0] = 0.85f; // iv→i (plagal pop)
            return p;
        }

        case StyleId::trap:
        {
            auto p = make (StyleId::trap, "Trap",
            {
                DegreeSpec { 1,  0, ChordQuality::minor,          extFlat7 },
                DegreeSpec { 2,  0, ChordQuality::halfDiminished, extFlat7 },
                DegreeSpec { 3, -1, ChordQuality::major,          extMajor7 },
                DegreeSpec { 4,  0, ChordQuality::minor,          extFlat7 },
                DegreeSpec { 5,  0, ChordQuality::sus4,           extFlat7 },
                DegreeSpec { 6, -1, ChordQuality::major,          extMajor7 },
                DegreeSpec { 7, -1, ChordQuality::dominant,       extNone }
            });
            // Trap loops hover: weaker cadence, flatter arc, repetition ok.
            p.tensionTargets = { 0.2f, 0.35f, 0.45f, 0.6f };
            p.cadenceWeight = 0.6f;
            p.brightness = 0.3f;
            for (int d = 0; d < 7; ++d)
                p.transitions[(size_t) d][(size_t) d] = 0.35f; // self-loops allowed
            p.transitions[5][3] = 0.85f; // bVI→iv
            p.transitions[6][5] = 0.8f;  // bVII→bVI
            return p;
        }

        case StyleId::reggaeton: // reserved for M7 — fall back to Modern R&B grammar
        case StyleId::modernRnB:
        default:
            return modernRnB();
    }
}

} // namespace morph
