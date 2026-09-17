# MORPH — ARCHITECTURE

## Targets (CMakeLists.txt)

| Target | Kind | Purpose |
|---|---|---|
| `Morph` | plugin | Standalone + VST3 + AU (`juce_add_plugin`, shared code) |
| `MorphTests` | console app | Zero-dep test harness, ctest target `morph_tests` |
| `MorphUISnapshot` | gui app | Offscreen editor renders for visual verification (`tools/`) |

Engine sources (`MORPH_ENGINE_SOURCES`) compile into every target; engine
code depends only on `juce_core` + `juce_audio_basics` (DECISIONS D2).

## Repository layout

```
src/
  engine/
    theory/        Pitch, Interval, Scale, KeyContext, Chord (formulas, roman)
    harmony/       TriggerInterpreter, HarmonyEngine, StyleProfile
    voicing/       Voicing types, VoicingEngine
    performance/   PerformanceTypes, PerformanceEngine
    progression/   Progression (4 slots of intent)
    EngineHost.*   The audio-thread orchestrator
  midi/            MidiScheduler, MidiExporter, ScheduledNote
  state/           MusicalPlaybackState + PlaybackStateBuffer (seqlock)
  plugin/          PluginProcessor, PluginEditor, AuditionSynth
  ui/              MorphChassis, design_system/, header/, radial/,
                   keyboard/, controls/
tests/             TestHarness + theory/harmony/voicing/performance/midi/
                   keyboard/integration suites
tools/             UISnapshotMain.cpp
docs/              This documentation set
```

## Threading model

- **Audio thread** owns `EngineHost`: input FIFO drain → engine pipeline →
  `MidiScheduler` (sample-accurate) → `MusicalPlaybackState` → publish via
  `PlaybackStateBuffer` (single-slot seqlock — no locks, §81).
- **Message thread** (UI): 30 Hz `juce::Timer` reads the snapshot and paints
  keyboard + radial field from it. UI never owns musical logic (§43).
- **Settings**: APVTS parameters are the persistent truth; the processor
  mirrors them into `EngineHost`'s atomics every block. Performance-mode
  changes go through `requestPerformanceMode` so an active chord is
  re-performed, not regenerated (§112).
- **UI note input**: lock-free SPSC ring (`UiNoteQueue`, capacity 64) from the
  on-screen keyboard into `processBlock`.

## Data flow (canonical pipeline, §43)

```
MIDI in (host) / UiNoteQueue (on-screen keys)
  → TriggerInterpreter (scale degree, chromatic snap)
  → HarmonyEngine (degree → chord identity, style vocabulary)
  → VoicingEngine (identity → voicing, register bands, low-end rules)
  → PerformanceEngine (TOGETHER/STRUM → scheduled events, deterministic RNG)
  → MidiScheduler (absolute-sample event store, lifecycle, sustain)
  → MusicalPlaybackState (generatedChordNotes / currentlySoundingNotes / …)
  → MIDI out buffer  +  UI snapshot (keyboard lights, orb, pucks, arcs)
```

Standalone only: `AuditionSynth` renders the MIDI output to audio so the
instrument is audible without a host (DECISIONS D10).

## Realtime safety

No heap allocation, no locks, no I/O in `processBlock`. Fixed-capacity
containers (`std::array`), preallocated scheduler store (256 events), seeded
xorshift RNG, sample-offset scheduling only (§59, §81). State handoff to the
UI is a wait-free seqlock copy (DECISIONS D4).
