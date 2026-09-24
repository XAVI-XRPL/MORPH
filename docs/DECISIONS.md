# MORPH — DECISIONS

Meaningful architecture decisions, newest last. Format: decision / rationale /
consequence.

## D1 — JUCE 8.0.12 pinned as a shallow git submodule
Reproducible builds; no reliance on system-wide installs. `add_subdirectory(JUCE)`.
Consequence: clones must use `git submodule update --init --depth 1`.

## D2 — Engine sources compile into both the plugin and the test runner
A shared `MORPH_ENGINE_SOURCES` list feeds `juce_add_plugin(Morph)` and
`juce_add_console_app(MorphTests)` instead of a separate static library.
Rationale: JUCE module header configuration (AppConfig vs global settings)
is handled per-target by the JUCE CMake API; a shared static lib would
duplicate that configuration machinery. Engine files depend only on
`juce_core` + `juce_audio_basics` and stay UI-free, so the test runner needs
no GUI modules.

## D3 — Zero-dependency test harness
`tests/TestHarness.h` (registry + CHECK macros) with ctest integration.
Rationale: no extra network dependency, full control over output, tests read
as plain functions. JUCE's own UnitTest framework ties tests to module
structure; we wanted suite-per-area layout from spec §87.

## D4 — One authoritative MusicalPlaybackState + seqlock snapshot
Audio thread is the single writer; UI reads a copy through a single-slot
seqlock (`PlaybackStateBuffer`). Rationale: spec §81 forbids locks in the
realtime callback; a seqlock copy of ~300 bytes at 30 Hz is trivially cheap
and gives the UI a consistent snapshot (§42 invariant is testable from it).

## D5 — Sample-accurate MidiScheduler with absolute clock
All performance events carry absolute sample offsets on a monotonic clock;
strums cross block boundaries correctly by construction. No timers, no
sleep, no blocking (§59). Event store is a fixed-capacity sorted array
(256 events) — insertion-ordered, shifted on consume; bounded work.

## D6 — Note lifecycle ownership in the scheduler
Groups: every scheduled chord gets a group id. Release cancels pending
attacks and note-offs sounding notes (sustain-aware). Retrigger cancels
obsolete pending events, releases non-common tones, adopts common tones
into the new group without re-attack (§60–§62). `allNotesOff` bypasses
sustain (panic semantics) so stuck notes are impossible.

## D7 — Deterministic humanization (xorshift32, seeded)
PerformanceEngine is a pure function:
same realization + profile + seed → identical events. Consequence: MIDI
export reproduces heard playback exactly (§79), and tests are exact.

## D8 — Monophonic live trigger
The latest input note owns the live chord; a new trigger retriggers
(common-tone preserving). Rationale: MORPH's defining interaction is
"press one note → one chord" (§03); polyphonic trigger interpretation is a
post-V1 question (SMART trigger mode).

## D9 — Register-band voicing recipe (M1), candidate search later (M3)
VoicingEngine places chord tones in deterministic register bands
(bass root oct.2, fifth above bass, 7th in oct.3, 9th/11th in oct.4,
3rd/4th on top) with top-voice octave continuity against the previous
voicing. This reproduces the golden Cm9 = C2 G2 Bb3 D4 Eb4 exactly and
keeps the low end clean (§49). Full candidate generation + scoring is
Milestone 3 by design.

## D10 — Standalone ships an audition synth
MORPH is a MIDI instrument; in Standalone there is no host instrument.
`AuditionSynth` (juce::Synthesiser, soft sine voice) renders the MIDI
output only when `JucePlugin_Build_Standalone`. DAW builds emit pure MIDI.

## D11 — AU builds and validates with Command Line Tools (no full Xcode)
Initially assumed deferred (CLT-only host), but the CLT macOS SDK ships the
AudioUnit framework headers, so the AU target builds. Verified with
`auval -t aumi Clam Mrp1` → "AU VALIDATION SUCCEEDED" (MORPH is an 'aumi'
MIDI processor AU). Formats: Standalone + VST3 + AU.

## D12 — 1440×900 design canvas, uniformly scaled
UI is laid out in fixed 1440×900 design coordinates inside `MorphChassis`;
the editor applies a uniform `AffineTransform::scale` (min 1080×675,
max 1680×1050, aspect locked). Rationale: §111 proportion preservation
with one source of layout truth; vector drawing scales cleanly.

## D13 — MORPH action button: deterministic voicing sibling (interim)
Milestone 4 wires the MORPH button to `EngineHost::requestMorphVariation`:
same chord identity, deterministic alternate voicing shape, re-performed in
the current mode. The full MorphEngine (harmony/voicing/bass/top/rhythm
siblings with locks and seeds) is Milestone 5. The interim behavior keeps
the button musically real without pre-empting M5.

## D14 — FEEL lists future styles disabled; KEY cycles 12 minor keys
V1 ships the Modern R&B StyleProfile. The FEEL popover shows planned styles
greyed out (honest surface, §17: advanced complexity stays hidden). KEY/SCALE
cycles all 12 minor keys — small, real, and complete for V1's minor
vocabulary. Major keys arrive with the style expansion milestone.

## D15 — MIDI export renders from intent, not from a recording
Export re-runs the same deterministic engine pipeline per progression slot
(same seeds as the sequencer) instead of capturing live MIDI. Timing equals
heard scheduling (tested, §91) and export works before anything was played.

## D15 — M3: VoicingEngine = candidate generation + weighted scoring
Register-band recipe remains the canonical no-context path (golden Cm9 is
unchanged); with previous-chord context the engine generates
(bass-choice × top-choice) candidates — bass from BassEngine, top from
TopVoiceEngine — and scores them for voice movement, common tones, leaps,
crossing, register, spacing, low-end cleanliness, bass quality and top-line
quality (§48). Stateless call signature: realize(candidate, key, style,
VoicingContext, variation).

## D16 — Voice-leading memory resets on silence
Voice leading applies within a connected phrase only: when nothing sounds
and nothing is held, `vlMemory` resets and the next chord starts canonical.
Prevents a dead chord from dragging the next phrase around (and keeps the
M2 golden-modes test honest).

## D17 — Flow bass never parks on a non-root
Flow minimizes bass movement among root/third/fifth in [36..47], but if the
winner would hold the previous pitch while that pitch is not the new chord's
root, it moves by step instead (§50: bass is melodic). Gold family result:
C2 → Eb2 → F2 → G2 — stepwise into the dominant. Pedal and bounce profiles
exist; motion maps still→root, mid→flow, moving→bounce.

## D18 — Sequence-level voicing plan for the 4-slot loop
PLAY builds a 4-slot plan (forward voice-leading pass + loop-closure pass on
slot 0 against slot 3) cached per settings signature (key/COLOR/SPACE/
MOTION/variation). The sequencer and MIDI export render from identical
plans, keeping export == playback. Top-line contour across the gold family:
D4 → Eb4 → Eb4 → D4.

## D19 — MORPH action = progression sibling (M5)
The MORPH button now morphs the 4-slot progression (stylistic degree
substitution tables, locked slots preserved, tonic anchor protected below
0.75 morph amount, anti-stagnation guarantees ≥1 change) and advances the
voicing variation. Deterministic: seed = morph counter; same seed + amount
→ same sibling (§66, §93).

## D20 — Progression state lives in an RT-safe double buffer
Message thread writes a copy and flips an atomic index; audio thread reads
the active buffer. A planDirty flag rebuilds the M3 voicing plan on the next
block. Undo/redo = two-stack snapshots of {progression, morphVariation},
message-thread only. Locks are slot state, not undoable actions.

## D21 — Puck lock via right-click, minimal visual marker
Locking keeps the main surface clean (§00): right-click a puck toggles its
lock; a thin ring + pin dot marks locked slots. No new permanent controls.

## D22 — Composition state persists inside the APVTS state tree
Progression degrees/locks + morphVariation ride as ValueTree properties, so
DAW session save/restore and the SAVE button both capture composition state.

## D23 — StyleProfile owns vocabulary AND grammar (M6)
Degree maps (quality/extensions per degree, major-lattice romans), transition
weight matrix, tension arc targets, cadence weight live in StyleProfile;
HarmonyEngine reads the map. Six implemented styles (Modern R&B, Dark R&B,
Neo-Soul, Emotional, Dark Pop, Trap); Reggaeton reserved for M7 (needs the
rhythmic engine).

## D24 — Beam search with per-seed grammar wobble
ProgressionGenerator: beamWidth 32, branchFactor 12 (§72). Search-time edge
weights are warped 0.8–1.2 per seed (diversity); final QualityMetrics use TRUE
style weights so bank scores stay comparable. Slot 0 start degree is seeded
(mostly tonic; sometimes bVI/iv/bIII color launches) with a tonic rival lineage.

## D25 — GOLD bank is generated, validated, deduplicated, compiled in
tools/MorphBankTool regenerates src/engine/bank/GoldBank.inc (300 entries:
50 per style × 6); tests/bank/BankTests.cpp re-derives every entry from its
seed (determinism), re-checks gold thresholds, and proves per-style
uniqueness. The bank ships compiled (no runtime I/O). Regenerate with:
`./build/MorphBankTool_artefacts/Release/MorphBankTool --generate --count=50`.

## D26 — MORPH stays substitution-based; the bank powers alternatives
MORPH (M5) keeps its tested deterministic substitution semantics. The M6
generator/bank feed EXPLORE → "Progression alternatives" (lock-preserving
merge). A generator-driven morph can be evaluated later against real usage.

## D27 — M8 streams share the chord's scheduler group
PULSE/PATTERN/ARP events are generated per window into the trigger's group
id, so the existing lifecycle (release-cancel, sustain deferral, retrigger
adoption, allNotesOff) covers streams with zero new lifecycle machinery.
Streams are pure functions of (realization, profile, tempo, absolute grid,
seed) — window boundaries never affect content; export renders identical
windows. UI: mode pill lists 6 modes; contextual submenus (Direction, Rate,
Pattern, Arp direction) appear only for the active mode (§5).
