# MORPH — ROADMAP

Contract milestones (MASTER_BUILD_PROMPT.md §95–§104) and status.

| MS | Title | Status | Acceptance evidence |
|---|---|---|---|
| 1 | Playable Foundation | **DONE** | C3 → Cm9 heard + exact notes + active chord; `golden.pressC3ProducesCm9`; Standalone/VST3/AU build |
| 2 | Performance | **DONE** | TOGETHER/STRUM↑/STRUM↓ same identity (`golden.sameChordAcrossPerformanceModes`); spread/curves/policies/lifecycle/sustain/retrigger tests; export timing test; sequential lighting verified in snapshots |
| 3 | Musical Quality | **PARTIAL** | Sequencer + top-voice continuity exist; pending: candidate voicing generation + scoring, BassEngine, TopVoiceEngine (§50–51) |
| 4 | Canonical UI | **DONE** | Reference reproduced (chassis/header/banks/radial/pucks/orb/arcs/action row/keyboard); verified via offscreen renders against docs/UI_REFERENCE_SPEC.md |
| 5 | Creative Workflow | Pending | morph macro engine, lock, undo/redo, save (APVTS save exists), explore, MIDI drag-out |
| 6 | Progression Intelligence | Pending | beam search (32/12), style profiles, 300 GOLD seeds, validation/dedup/audition |
| 7 | Commercial Bank | Pending | style families per §101; songs over count |
| 8 | Advanced Performance | Pending | PULSE/PATTERN/(ARP) only after strum excellence (strum is tested-excellent) |
| 9 | Song Kit | Pending | SongDNA/PerformanceDNA/sections; 10 → 100 → 250+ GOLD kits |
| 10 | Song Workflow | Pending | BUILD SONG etc.; main UI unchanged |

## Next recommended milestone

**M3 — Musical Quality.** The truth pipeline and the canonical surface are
proven; deepening voicing candidates, voice-leading optimization, BassEngine
and TopVoiceEngine raises musicality before workflow breadth. The sequencer
and progression memory already in place give M3 an immediate audible target.

## Current known limitations

- Live trigger is monophonic (DECISIONS D8); SMART/poly triggers are post-V1.
- Voicing uses the register-band recipe (D9) until M3's candidate scoring.
- MORPH action performs deterministic voicing siblings (D13); full
  MorphEngine + locks + undo arrive with M5.
- MIDI export covers the 4-slot progression and current chord; stems/sections
  arrive with Song Kit milestones.
- AU is built and auval-validated on this host (DECISIONS D11); DAW-specific
  hosting quirks should be validated per-host as part of release QA.
