#pragma once

#include "../engine/progression/Progression.h"
#include <vector>

namespace morph
{

/** Composition state snapshot for undo checkpoints (§68). */
struct CompositionSnapshot
{
    Progression progression;
    int morphVariation = 0;

    bool operator== (const CompositionSnapshot& other) const
    {
        if (morphVariation != other.morphVariation)
            return false;
        for (int i = 0; i < Progression::numSlots; ++i)
            if (progression.slots[(size_t) i].degree.value != other.progression.slots[(size_t) i].degree.value
                || progression.slots[(size_t) i].locked != other.progression.slots[(size_t) i].locked)
                return false;
        return true;
    }
};

/**
 * Two-stack undo/redo for composition actions (§68). Deterministic restore.
 * Message-thread only (allocations allowed here — never on the audio thread).
 */
class UndoHistory
{
public:
    /** Call BEFORE a destructive action with the pre-action state. */
    void checkpoint (const CompositionSnapshot& before)
    {
        past.push_back (before);
        future.clear();
    }

    bool canUndo() const { return ! past.empty(); }
    bool canRedo() const { return ! future.empty(); }

    /** Returns the state to restore; `current` becomes redoable. */
    CompositionSnapshot undo (const CompositionSnapshot& current)
    {
        future.push_back (current);
        auto s = past.back();
        past.pop_back();
        return s;
    }

    CompositionSnapshot redo (const CompositionSnapshot& current)
    {
        past.push_back (current);
        auto s = future.back();
        future.pop_back();
        return s;
    }

private:
    std::vector<CompositionSnapshot> past;
    std::vector<CompositionSnapshot> future;
};

} // namespace morph
