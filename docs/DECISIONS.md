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
