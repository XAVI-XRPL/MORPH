# MORPH — PRODUCT_BEHAVIOR

What the product does today (Milestones 1+2+4). Contract: MASTER_BUILD_PROMPT.md.

## Implemented behavior

**One note → one chord (§3, §109.1).** Pressing a single note (MIDI host input
or the on-screen keyboard) interprets the note as a scale degree in the current
key (default C minor, SCALE_DEGREE trigger mode), selects the chord via the
Modern R&B style vocabulary, voices it (register-band voicing with low-end
cleanliness), and performs it through the current Performance Mode. Golden
case: C3 → i → Cm9 → C2 G2 Bb3 D4 Eb4 (bass C2, top Eb4).

**Performance modes (§4, §96).** TOGETHER (default), STRUM UP, STRUM DOWN.
Switching modes never regenerates the chord or voicing — if a chord is active,
the same realization is re-performed in the new mode (§112 flow). Strum
contextual controls (spread ±10 ms steps) live in the PERFORMANCE pill menu,
not on the main surface (§5).

**Knobs (left bank).** COLOR dark↔bright extension density (7th-only →
9th → +11th on minor functions). MOTION still↔moving scales strum spread
(±30%) — never overrides explicit performance mode. MORPH knob is wired as a
parameter; the full sophistication macro is Milestone 5.

**Knobs (right bank).** SPACE close↔open voicing openness (thresholds at
0.33/0.66). TEXTURE clean↔textured humanization amounts (deterministic,
seeded). OUTPUT MIDI velocity baseline (48–120) — MIDI intensity, not audio
volume (§21).

**Progression memory + PLAY (§31, §63, §64).** Four slots hold musical intent
(scale degrees): i9 → bVImaj9 → iv9 → V7sus (the §106 gold family). PLAY
sequences one chord per bar at host tempo (80 BPM in standalone), realized
through the same engine; the active slot puck lights, the orb shows the current
chord, the keyboard shows exactly the sounding MIDI notes.

**Live override (§65).** While the sequencer plays, holding a note suppresses
sequenced output; releasing resumes at the next bar boundary.

**MORPH action (M5).** The MORPH button creates a deterministic musical sibling
of the progression: stylistic degree substitutions on unlocked slots (tonic
anchor protected below 0.75 morph amount), voicing variation advances, locked
slots never change. Right-click a progression puck to lock/unlock it (ring +
pin marker). UNDO/REDO restore exact composition state. EXPLORE offers
progression variations and the curated performance presets (§58). MIDI
supports save-dialog export and drag-out into the DAW.

**KEY / SCALE (§11).** Segmented pill: arrows step through 12 minor keys,
center opens the selection menu. FEEL shows Modern R&B; the popover lists
planned styles as disabled (§17).

**SAVE.** Writes the APVTS state to `~/Documents/MORPH/morph-state.xml`.
UNDO/REDO are visible and intentionally dimmed (M5). EXPLORE/MORE show
honest popovers for deferred functionality.

**MIDI export (§33, §79).** MIDI button exports the 4-slot progression
performance to a `.mid` file (save dialog). Timing equals heard scheduling
(same deterministic render path).

## Deferred to later milestones

Progression generation/beam search/bank (M6/M7), PULSE/PATTERN/ARP (M8),
Song Kit (M9/M10), capture (§80).
