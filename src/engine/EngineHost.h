#pragma once

#include "theory/KeyContext.h"
#include "harmony/TriggerInterpreter.h"
#include "harmony/HarmonyEngine.h"
#include "voicing/VoicingEngine.h"
#include "performance/PerformanceEngine.h"
#include "progression/Progression.h"
#include "morph/MorphEngine.h"
#include "bank/ProgressionBank.h"
#include "../midi/MidiScheduler.h"
#include "../midi/MidiExporter.h"
#include "../state/MusicalPlaybackState.h"
#include <atomic>

namespace morph
{

/**
 * EngineHost: the canonical engine pipeline (spec §43), audio-thread resident.
 *
 *   MIDI INPUT → TriggerInterpreter → KeyContext → HarmonyEngine
 *   → VoicingEngine → ChordRealization → PerformanceEngine
 *   → MidiScheduler → MusicalPlaybackState → MIDI OUTPUT
 *
 * All settings are atomics written by the message thread; the audio thread
 * reads them lock-free. All engine work is bounded and allocation-free.
 */
class EngineHost
{
public:
    EngineHost();

    void prepare (double sampleRate, int maxBlockSize);

    // --- Settings (message thread, atomic) ---
    std::atomic<int> keyIndex { 0 };                          // 12 minor keys
    std::atomic<int> styleIndex { 0 };                        // StyleId (M6 FEEL)
    std::atomic<int> performanceMode { (int) PerformanceMode::together };
    std::atomic<int> togetherKind { (int) TogetherKind::tight };
    std::atomic<int> strumCurve { (int) StrumCurve::human };
    std::atomic<int> strumVelocityShape { (int) StrumVelocityShape::rise };
    std::atomic<int> bassPolicy { (int) BassStrumPolicy::withStrum };
    std::atomic<int> topVoicePolicy { (int) TopVoicePerformancePolicy::normal };
    std::atomic<float> strumSpreadMs { 42.0f };

    std::atomic<float> colorKnob { 0.5f };   // harmonic brightness
    std::atomic<float> motionKnob { 0.5f };  // strum/performance movement
    std::atomic<float> morphKnob { 0.5f };   // creative macro (M5)
    std::atomic<float> spaceKnob { 0.4f };   // voicing openness
    std::atomic<float> textureKnob { 0.2f }; // humanization amount
    std::atomic<float> outputKnob { 0.75f }; // MIDI velocity baseline

    /** UI requests a performance-mode change. If a chord is active, the SAME
        realization is re-performed in the new mode (spec §112). */
    void requestPerformanceMode (PerformanceMode mode);

    /** MORPH action (M4 wiring; full MorphEngine is M5): selects the next
        deterministic voicing sibling and re-performs the active chord. */
    void requestMorphVariation();

    /** Sequencer transport (message thread). */
    void play();
    void stop();

    // --- Composition editing (message thread; RT-safe swap to audio) ---

    /** MORPH action (§30, §66): deterministic sibling of the progression,
        locked slots preserved; also advances the voicing variation. */
    void morphProgressionFromUi();

    /** Toggle a slot's lock (§67). Locked slots never morph. */
    void toggleSlotLockFromUi (int slot);

    /** Apply a GOLD bank entry: unlocked slots take the entry's degrees,
        locked slots keep theirs (§9 discipline, §67). */
    void applyBankEntryFromUi (const BankEntry& entry);

    /** Replace the whole progression (undo/redo restore). */
    void setProgressionFromUi (const Progression& p);

    /** UI view of the active progression (message thread). */
    Progression getProgressionForUi() const;

    /** Voicing-variation counter (undo/redo + persistence). */
    int getMorphVariation() const { return morphVariation.load (std::memory_order_relaxed); }
    void setMorphVariation (int v)
    {
        morphVariation.store (v, std::memory_order_relaxed);
        planDirty.store (true, std::memory_order_release);
    }

    /** Bumped on every progression swap (UI watches for changes). */
    std::atomic<uint32_t> progressionVersion { 0 };

    /** Host tempo (audio thread, cheap). */
    void setTempoBpm (double bpm) { tempoBpm = bpm > 0.0 ? bpm : 80.0; }

    // --- Audio-thread entry points (offsets relative to current block) ---
    void noteOn (int inputPitch, float velocity, int64_t offsetWithinBlock);
    void noteOff (int inputPitch, int64_t offsetWithinBlock);
    void sustainPedal (bool down, int64_t offsetWithinBlock);

    void processBlock (juce::MidiBuffer& out, int numSamples);

    // --- State ---
    PlaybackStateBuffer& stateBuffer() { return stateSnapshot; }
    const MusicalPlaybackState& audioState() const { return state; }
    const Progression& getProgression() const { return activeProgression(); }

    /** Deterministic render of a slot degree (used by sequencer + export). */
    ChordRealization realizeDegree (ScaleDegree degree, const VoiceLeadingMemory& mem,
                                    int slotIndex) const;

    /** Builds the 4-slot voicing plan (sequence-level optimization, §48):
        forward pass threading voice-leading memory, then a loop-closure pass
        on slot 0 against slot 3. Stateless — safe on any thread. */
    void buildProgressionPlan (std::array<ChordRealization, 4>& out) const;

    // --- Export (message thread only) ---
    /** Renders the current progression's performance for MIDI export (§79). */
    std::vector<MidiExporter::ChordEvent> renderProgressionPerformance();
    /** Renders the most recent live chord performance for MIDI export. */
    std::vector<MidiExporter::ChordEvent> renderCurrentChordPerformance();

private:
    PerformanceProfile buildProfile() const;
    KeyContext currentKey() const;
    void scheduleLive (const ChordRealization& r, int64_t originSample);
    void scheduleSequencerSlot (int slot, int64_t barStart, int64_t barLength);
    void updateGeneratedState (const ChordRealization& r, const ScheduledNoteList<maxChordVoices>& notes);
    void clearIfSilent();

    double sampleRate = 44100.0;
    int blockSize = 512;

    TriggerInterpreter interpreter;
    HarmonyEngine harmony;
    VoicingEngine voicingEngine;
    MorphEngine morphEngine;

    StyleProfile currentStyle() const
    {
        return StyleProfile::get ((StyleId) styleIndex.load (std::memory_order_relaxed));
    }

    // Progression double buffer: audio thread reads activeProgression(),
    // message thread writes via setProgressionFromUi (copy → flip).
    std::array<Progression, 2> progressionBuffers {};
    std::atomic<int> progressionIndex { 0 };
    std::atomic<bool> planDirty { false };
    std::atomic<uint32_t> morphCounter { 0 };

    const Progression& activeProgression() const
    {
        return progressionBuffers[(size_t) progressionIndex.load (std::memory_order_acquire)];
    }

    MidiScheduler scheduler;

    // Live trigger (monophonic: latest input owns the live chord)
    int liveInputPitch = -1;
    int liveGroupId = -1;
    ChordRealization lastRealization;
    bool hasLastRealization = false;
    VoiceLeadingMemory vlMemory;

    // Sequence-level voicing plan for progression playback (audio thread)
    std::array<ChordRealization, 4> slotPlan {};
    bool planValid = false;
    uint64_t planSignature = 0;
    uint64_t computePlanSignature() const;
    void rebuildPlanIfNeeded();

    // Mode re-performance
    std::atomic<int> pendingMode { -1 };

    // MORPH action (deterministic voicing sibling)
    std::atomic<int> morphVariation { 0 };
    std::atomic<bool> morphPending { false };
    int lastDegree = 1;

    // Sequencer
    std::atomic<int> transportCommand { 0 }; // 0 none, 1 play, 2 stop
    bool sequencerPlaying = false;
    int64_t nextBarSample = 0;
    int currentSlot = 0;
    int sequencerGroupId = -1;
    bool resumeAtNextBar = false;
    double tempoBpm = 80.0;

    // Strum progress tracking
    int64_t activeGroupOrigin = 0;
    int64_t activeSpreadSamples = 1;
    int activeGroupNoteCount = 0;

    MusicalPlaybackState state;
    PlaybackStateBuffer stateSnapshot;

    uint32_t triggerCounter = 0;
};

} // namespace morph
