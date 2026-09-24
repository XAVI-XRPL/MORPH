# MORPH — PERFORMANCE_ENGINE

`src/engine/performance/` — turns a `ChordRealization` into scheduled MIDI.
Controls attack order/timing/velocity/duration; never selects harmony (§52).

## Types (§53–§57)

- `PerformanceMode`: together | strumUp | strumDown (pulse/pattern/arp reserved)
- `TogetherKind`: tight (0.8 ms) | soft (2 ms) | human (3.5 ms) | wide (6 ms)
- `StrumCurve`: linear | easeIn | easeOut | human (bounded musician-like
  variation, monotonicity enforced — not random jitter, §55)
- `StrumVelocityShape`: flat | rise | fall
- `BassStrumPolicy`: withStrum | anchorFirst | anchorSimultaneous | delayed |
  excluded (StyleProfile recommends anchorFirst for Modern R&B; user wins §56)
- `TopVoicePerformancePolicy`: normal | arriveLast | arriveFirst | accent | hold
- `StrumProfile` carries direction/spreadMs/curve/velocityShape/policies/
  randomTiming/randomVelocity (§54)

## Scheduling semantics

- `PerformanceEngine::schedule(realization, profile, sampleRate,
  noteLengthSamples, seed)` → fixed-capacity `ScheduledNoteList` with
  sample-accurate on/off offsets relative to the trigger.
- **TOGETHER**: all attacks at t=0 plus tiny bounded offsets inside the 5 ms
  tolerance (§91); TEXTURE widens offsets within tolerance.
- **STRUM**: pitch-sorted attack order (ascending for UP, descending for
  DOWN); policies extract bass/top first, then the order is reassembled;
  position i of n maps through the curve to `spreadMs` (first-to-last =
  spread, tested). Rise/fall shapes scale velocity along the order.
- **Role velocity weights**: bass −4, inner 0, top +6; accent policy adds +12
  to the top; hold extends its length (finite-length renders).
- **Determinism**: xorshift32 seeded per trigger — same realization + profile +
  seed → identical events (export reproduces playback, §79, §93).

## Knob wiring

- MOTION: strum spread × (1.3 − 0.6·motion) — still is tighter (§16).
- TEXTURE: humanize timing/velocity amounts (§20).
- OUTPUT: velocity baseline 48–120 (§21).

## Note release / retrigger (§60, §61)

Durations are −1 (until release) for live play; the sequencer uses finite
bar-length notes. Release before strum end = CANCEL_PENDING_ATTACKS (default;
COMPLETE_STRUM reserved). Retrigger: cancel pending, release non-common tones,
adopt common tones without re-attack (implemented by EngineHost +
MidiScheduler group primitives).

## Stream modes (M8, §102): PULSE / PATTERN / ARP

- New `PerformanceMode` values: pulse, pattern, arp (isStreamMode helper).
- `PatternEngine` generates events per absolute-sample window from
  (realization, profile, tempo, stream start, seed) — a pure function, so
  refills == continuation and export == playback. Events join the SAME
  scheduler group as the trigger's chord, so release/retrigger/stop kill the
  whole stream through the existing group lifecycle (no stuck notes — tested).
- PULSE: full-chord re-strikes on the grid (1/8, 1/16, 1/8T), accentEvery,
  gate ratio, TEXTURE adds swing; optional bass hold (MOTION low).
- ARP: cycles chord tones (octave-expanded via MOTION), up/down/up-down,
  rate shared with PULSE, optional top hold.
- PATTERN: curated one-bar loops on an eighth grid — BOUNCE (bass anchors +
  offbeat chord answers), FLOAT (slow swell, top holds), STAB (syncopated
  hits).
- Strum directions (§54): up, down, up-down, down-up, outside-in, inside-out,
  controlled-random (seeded). Contextual Direction menu under the performance
  pill (§5), never permanent surface controls.
- Sequencer runs streams per bar (one stream per slot, tempo-synced to host);
  live override and release/resume semantics unchanged.
