# MORPH — Implementation Plan (Milestones 1 + 2, then 4)

Per MASTER_BUILD_PROMPT_V2.0 §114. This plan covers the engine-truth pipeline
first (M1, M2), then the canonical UI (M4).

## 1. Project structure

```
Morph/
├── CMakeLists.txt            # root: plugin + test targets
├── JUCE/                     # submodule, pinned 8.0.12 (shallow)
├── src/
│   ├── engine/
│   │   ├── theory/           # Pitch, Interval, Scale, KeyContext, Chord, Roman
│   │   ├── harmony/          # TriggerInterpreter, HarmonyEngine
│   │   ├── voicing/          # Voice/Voicing types, VoicingEngine
│   │   ├── performance/      # PerformanceMode/Profiles, PerformanceEngine
│   │   └── progression/      # ProgressionSlot/Progression, default gold family
│   ├── midi/                 # ScheduledNote, MidiScheduler, MidiLifecycle, MidiExporter
│   ├── state/                # MusicalPlaybackState (+ RT→UI snapshot buffer)
│   ├── plugin/               # PluginProcessor, PluginEditor
│   └── ui/                   # design_system, header, radial, keyboard, controls
├── tests/                    # custom zero-dep harness + ctest
│   ├── theory/ harmony/ voicing/ performance/ midi/ keyboard/ integration/
├── docs/
└── resources/
```

Engine code is plain C++20 with minimal JUCE deps (`juce_core`,
`juce_audio_basics`). UI never owns musical logic.

## 2. JUCE / CMake setup

- JUCE 8.0.12 as a pinned git submodule (`JUCE/`), `add_subdirectory(JUCE)`.
- `juce_add_plugin(Morph)`: formats **Standalone + VST3 (+ AU if the CLT SDK
  supports it)**, `NEEDS_MIDI_INPUT`, `PRODUCES_MIDI_OUTPUT`,
  `IS_MIDI_EFFECT TRUE`, C++20.
- `juce_add_console_app(MorphTests)` + `ctest`; engine sources are compiled
  into both targets via a shared `MORPH_ENGINE_SOURCES` list (no separate
  static lib — avoids JUCE module config duplication).
- Standalone gets a small built-in audition synth (MORPH outputs MIDI; the
  standalone must still let you *hear* a chord). Documented in DECISIONS.md.
- Parameters via `AudioProcessorValueTreeState` (key, performance mode,
  strum params, the six knobs).

## 3. Core data types (strong typing, no strings as musical state)

`PitchClass` (0–11), `MidiPitch` (0–127), `Interval`, `ScaleDegree` (1–7),
`ScaleDefinition`, `KeyContext` (tonic + scale), `ChordQuality`,
`ChordExtension` set, `ChordSymbol`, `ChordFormula`, `RomanFunction`,
`ChordCandidate`, `ChordRealization` (voices with `VoiceRole`: Bass / Inner /
Top), `PerformanceMode` (Together / StrumUp / StrumDown),
`StrumProfile` (direction, spreadMs, curve, velocityShape, bassPolicy,
topVoicePolicy, humanization), `ScheduledNote` (pitch, velocity,
noteOn/off sample offsets, role), `ProgressionSlot`, `Progression`.

## 4. MIDI architecture

`MidiScheduler` — realtime-safe, fixed-capacity (preallocated arrays, no heap
in the audio callback, no locks, no timers):

- Input triggers (host MIDI or on-screen keyboard) enter through a lock-free
  SPSC FIFO, consumed at block start with sample offsets.
- Engine runs synchronously (bounded, preallocated work): Trigger → Harmony →
  Voicing → Performance → `ScheduledNote` list with sample-accurate offsets.
- Scheduler emits note-ons/offs as they come due across blocks; tracks
  pending attacks, sounding set, sustain (CC64), retrigger (cancel obsolete
  pending, release non-common tones), release-before-strum-end
  (`CANCEL_PENDING_ATTACKS`).
- `MidiExporter` renders the same scheduled events to a Standard MIDI File —
  export timing == heard timing.

## 5. MusicalPlaybackState

One authoritative struct (fixed arrays, no allocation): inputNotes,
generatedChordNotes, currentlySoundingNotes, pendingScheduledNotes,
activeChord, activeRomanFunction, activeProgressionSlot, bassPitch, topPitch,
innerVoices, performanceMode, strumDirection, strumProgress, currentBeat/Bar,
isSequencerPlaying, isLiveOverride, sustainState, velocityData.

Audio thread is the single writer; UI reads via a lock-free seqlock snapshot
(triple buffer), pulled at ~30 Hz by a UI timer (repaint only — timers are
never used for MIDI scheduling).

**Core truth invariant** (tested): at any playback moment, actual sounding
MIDI == `currentlySoundingNotes` == keyboard-lit notes.

## 6. Realtime strategy

No heap allocation, no mutexes, no I/O in `processBlock`. Fixed-capacity
containers (`std::array` + size counters), deterministic xorshift RNG (seeded)
for humanization so export reproduces playback exactly. All scheduling in
sample offsets; spread ms → samples via the current sample rate.

## 7. Keyboard geometry

Pure function: MIDI range → key layout. Naturals {0,2,4,5,7,9,11},
accidentals {1,3,6,8,10}, black keys only between in-range naturals (never
E/F, B/C), groups of 2+3. Lighting maps **exact MIDI pitches** (not pitch
classes) to per-key light state colored by `VoiceRole`
(bass=blue, inner=orange, top=yellow). Unit-tested.

## 8. Radial Field state model

Center orb ← activeChord + sounding state (dim warm tonal-center glow when
idle). Four pucks ← `Progression` slots (default gold family:
Cm9 → Abmaj9 → Fm9 → G7sus/alt). Puck highlight ← activeProgressionSlot or
live-chord match (live chords never overwrite memory). Arcs/paths derive from
voice roles. M1: minimal but truthful; M4: full canonical treatment.

## 9. Test infrastructure

Zero-dependency harness (registry + `CHECK` macros) in a console app, one
ctest target. Suites: theory, harmony, voicing (incl. low-end cleanliness),
performance (together tolerance, strum order/spread/curves, velocity shape),
midi scheduler (release-cancel, retrigger, sustain, stuck notes), keyboard
geometry, integration (golden Cm9 scenario + truth invariant at every block
boundary).

## 10. Milestone sequencing

1. **M1** — engine pipeline + minimal UI; acceptance: press C3 → hear Cm9,
   see exact notes, correct active chord.
2. **M2** — Strum Up/Down, spread/curves, velocity shaping, bass/top
   policies, lifecycle (sustain/retrigger), sequential lighting, MIDI export;
   acceptance: same chord/voicing across TOGETHER/STRUM↑/STRUM↓.
3. **M4** — canonical UI reproduction against the locked reference (chassis,
   header, knob banks, radial field, action row, premium keyboard).
