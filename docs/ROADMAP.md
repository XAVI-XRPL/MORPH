# MORPH — ROADMAP

Contract milestones (MASTER_BUILD_PROMPT.md §95–§104) and status.

| MS | Title | Status | Acceptance evidence |
|---|---|---|---|
| 1 | Playable Foundation | **DONE** | C3 → Cm9 heard + exact notes + active chord; `golden.pressC3ProducesCm9`; Standalone/VST3/AU build |
| 2 | Performance | **DONE** | TOGETHER/STRUM↑/STRUM↓ same identity (`golden.sameChordAcrossPerformanceModes`); spread/curves/policies/lifecycle/sustain/retrigger tests; export timing test; sequential lighting verified in snapshots |
| 3 | Musical Quality | **DONE** | Candidate voicings + voice-leading scoring (§48), BassEngine (root/flow/pedal/bounce, §50), TopVoiceEngine (contour/stepwise/leap control, §51), sequence-level 4-slot plan with loop closure; plan dump: bass C2→Eb2→F2→G2, top D4→Eb4→Eb4→D4 |
| 4 | Canonical UI | **DONE** | Reference reproduced (chassis/header/banks/radial/pucks/orb/arcs/action row/keyboard); verified via offscreen renders against docs/UI_REFERENCE_SPEC.md |
| 5 | Creative Workflow | **DONE** | MORPH = deterministic progression sibling (lock-aware); right-click puck locks; undo/redo exact restore; performance presets (§58) in EXPLORE; MIDI drag-out + save dialog; acceptance: locked slot identical after morph |
| 6 | Progression Intelligence | Pending | beam search (32/12), style profiles, 300 GOLD seeds, validation/dedup/audition |
| 7 | Commercial Bank | Pending | style families per §101; songs over count |
| 8 | Advanced Performance | Pending | PULSE/PATTERN/(ARP) only after strum excellence (strum is tested-excellent) |
| 9 | Song Kit | Pending | SongDNA/PerformanceDNA/sections; 10 → 100 → 250+ GOLD kits |
| 10 | Song Workflow | Pending | BUILD SONG etc.; main UI unchanged |

## Next recommended milestone

**M6 — Progression Intelligence** (functional generation, beam search,
300 GOLD seeds, style profiles): the workflow loop (PRESS → MORPH → KEEP)
is proven; M6 deepens the vocabulary that MORPH and PLAY draw from.

## Current known limitations

- Live trigger is monophonic (DECISIONS D8); SMART/poly triggers are post-V1.
- AU is built and auval-validated on this host (DECISIONS D11); DAW-specific
  hosting quirks should be validated per-host as part of release QA.
