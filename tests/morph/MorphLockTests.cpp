#include "../TestHarness.h"
#include "engine/morph/MorphEngine.h"
#include "engine/EngineHost.h"
#include "state/UndoHistory.h"

using namespace morph;

MORPH_TEST (morph, deterministicSameSeed)
{
    // §93: same seed + same parameters = same result.
    MorphEngine engine;
    Progression p;

    const auto a = engine.morphProgression (p, 0.6f, 12345);
    const auto b = engine.morphProgression (p, 0.6f, 12345);

    for (int i = 0; i < 4; ++i)
        CHECK_EQ (a.slots[(size_t) i].degree.value, b.slots[(size_t) i].degree.value);
}

MORPH_TEST (morph, lockedSlotNeverMutates)
{
    // §99 acceptance: lock one slot, morph, unlocked may change, locked identical.
    MorphEngine engine;
    Progression p;
    p.slots[2].locked = true; // slot 3 (iv)
    const int lockedDegree = p.slots[2].degree.value;

    for (uint32_t seed = 1; seed <= 20; ++seed)
    {
        const auto out = engine.morphProgression (p, 1.0f, seed * 7919u);
        CHECK_EQ (out.slots[2].degree.value, lockedDegree);
        CHECK (out.slots[2].locked);
    }
}

MORPH_TEST (morph, unlockedMaterialCanMutate)
{
    MorphEngine engine;
    Progression p;

    bool anyDifferent = false;
    for (uint32_t seed = 1; seed <= 10; ++seed)
    {
        const auto out = engine.morphProgression (p, 0.8f, seed * 104729u);
        for (int i = 0; i < 4; ++i)
            if (out.slots[(size_t) i].degree.value != p.slots[(size_t) i].degree.value)
                anyDifferent = true;
    }
    CHECK (anyDifferent);
}

MORPH_TEST (morph, zeroAmountBarelyChanges)
{
    MorphEngine engine;
    Progression p;
    const auto out = engine.morphProgression (p, 0.0f, 42);

    int changed = 0;
    for (int i = 0; i < 4; ++i)
        if (out.slots[(size_t) i].degree.value != p.slots[(size_t) i].degree.value)
            ++changed;
    CHECK (changed <= 1); // the anti-stagnation rule changes at most one slot
}

MORPH_TEST (morph, substitutionsStayDiatonic)
{
    // §66/§46: morphing never produces out-of-vocabulary degrees.
    MorphEngine engine;
    Progression p;
    for (uint32_t seed = 1; seed <= 50; ++seed)
    {
        const auto out = engine.morphProgression (p, 1.0f, seed * 31u + 7u);
        for (int i = 0; i < 4; ++i)
        {
            const int d = out.slots[(size_t) i].degree.value;
            CHECK (d >= 1 && d <= 7);
        }
    }
}

MORPH_TEST (undo, checkpointRestoreRedo)
{
    // §93: undo restores exact previous state; redo restores exact new state.
    UndoHistory history;

    CompositionSnapshot a; // default gold family
    CompositionSnapshot b;
    b.progression.slots[1].degree = ScaleDegree { 3 };
    b.morphVariation = 2;

    history.checkpoint (a);
    CHECK (history.canUndo());
    CHECK (! history.canRedo());

    const auto restored = history.undo (b);
    CHECK (restored == a);
    CHECK (history.canRedo());

    const auto forward = history.redo (a);
    CHECK (forward == b);
    CHECK (! history.canRedo());
}

MORPH_TEST (undo, newActionClearsRedo)
{
    UndoHistory history;
    CompositionSnapshot a, b, c;
    b.progression.slots[0].degree = ScaleDegree { 6 };
    c.progression.slots[0].degree = ScaleDegree { 3 };

    history.checkpoint (a);
    auto back = history.undo (b);      // back to a; b redoable
    CHECK (back == a);
    history.checkpoint (a);            // new action from a
    CHECK (! history.canRedo());       // redo for b is gone
}

MORPH_TEST (morphIntegration, engineLockSurvivesMorph)
{
    // End-to-end through EngineHost: lock slot 1 (bVI), morph repeatedly,
    // slot 1 stays bVI; the plan rebuilds; playback stays truthful.
    EngineHost host;
    host.prepare (48000.0, 128);

    host.toggleSlotLockFromUi (1);

    const auto before = host.getProgressionForUi();
    CHECK (before.slots[1].locked);
    CHECK_EQ (before.slots[1].degree.value, 6);

    for (int i = 0; i < 5; ++i)
        host.morphProgressionFromUi();

    const auto after = host.getProgressionForUi();
    CHECK (after.slots[1].locked);
    CHECK_EQ (after.slots[1].degree.value, 6);

    // Plan rebuild reflects the morphed progression.
    std::array<ChordRealization, 4> plan;
    host.buildProgressionPlan (plan);
    for (int i = 0; i < 4; ++i)
        CHECK (plan[(size_t) i].valid);
}
