# MORPH — TESTING

## Running

```sh
cmake --build build --target MorphTests
ctest --test-dir build --output-on-failure
# or directly:
./build/MorphTests_artefacts/Release/MorphTests
```

Current: **42 suites, ~2400 checks, all passing.**

## Harness

Zero-dependency (`tests/TestHarness.h`): `MORPH_TEST(suite, name)` registers
into a static registry; `CHECK` / `CHECK_EQ` report file:line + expression;
`main` runs all and returns non-zero on any failure (wired to ctest).
Rationale in DECISIONS D3.

## Suite inventory

**theory/** — pitch-class math & wrap, C4=60 conventions, natural-minor
intervals, C-minor key context (degree mapping, membership, chromatic
rejection), chord formula compilation (incl. implied b7 on dominant/
half-diminished), roman numeral rendering (bVI, iiø, …), symbol rendering
(Cm9, Abmaj9, Fm9, G7sus).

**harmony/** — SCALE_DEGREE mapping in C minor (§45 table), chromatic input
snap-down policy (§46), Modern R&B vocabulary per degree, COLOR brightness
tiers.

**voicing/** — golden Cm9 realization exactly (C2 G2 Bb3 D4 Eb4, roles,
bass/top), gold family i9→bVImaj9→iv9→V7sus with ascending/ bass-register
invariants, low-end cleanliness (§49: nothing below 36, ≤2 voices below 48),
SPACE openness spans, morph-variation identity preservation + determinism.

**performance/** — TOGETHER tolerance (≤5 ms), strum up ascending / down
descending attack order, spread = first-to-last timing, curve shapes
(ease-in pulls mid attacks earlier, same endpoints), determinism (same seed),
bass anchor-first (bass leads even in strum down), top-voice arrive-last,
velocity rise shape.

**midi/** — events across block boundaries, release-before-strum-end cancels
pending (exactly the fired attacks get released), sustain deferral + release,
common-tone-preserving retrigger, allNotesOff panic (no stuck notes),
export timing == scheduling (tick-level), exported strum order.

**keyboard/** — accidental classes == {1,3,6,8,10}, no black key between
E/F or B/C, 2+3 grouping per octave, ascending MIDI order + count, exact
pitch highlighting (C2 G2 Bb3 D4 Eb4 light; other octaves do not).

**integration/** — `golden.pressC3ProducesCm9` (full §105 truth: chord,
roman, roles, state), `golden.sameChordAcrossPerformanceModes` (§96
acceptance), `truth.*` — the §42 harness: a shadow truth set built purely
from emitted MIDI is compared against `currentlySoundingNotes` AND the
keyboard-lit set at every block boundary, through strum unfold, retrigger,
sustain juggling, and sequencer playback; plus slot-advance and loop-wrap
checks.

## Adding tests

Drop a file in the matching `tests/` area, add it to `MorphTests` sources in
CMakeLists.txt, use `MORPH_TEST`. UI-state visual checks belong to
`tools/UISnapshotMain.cpp` renders (compare against docs/UI_REFERENCE_SPEC.md).
