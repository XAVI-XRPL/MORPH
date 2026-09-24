# MORPH — PROGRESSION_BANK

## Status (M6 implemented)

The progression system has two layers:

1. **Live progression memory** — 4 slots of musical intent (scale degrees),
   default gold family `i9 → bVImaj9 → iv9 → V7sus` (§106), morphed by the
   MORPH action (substitution tables, lock-aware, §66).
2. **GOLD seed bank** — 300 validated generated progressions, compiled in
   (`src/engine/bank/GoldBank.inc`), surfaced under EXPLORE →
   "Progression alternatives".

## Generation machinery (§72)

`ProgressionGenerator` — sequence-aware beam search:

- beamWidth 32, branchFactor 12 (7 diatonic degrees expanded per step)
- Slot 0 seeded start (mostly tonic; sometimes bVI/iv/bIII color launches,
  with a tonic rival lineage kept)
- Per-seed grammar wobble (edge weights ×0.8–1.2) for search diversity;
  final scoring always uses true style weights (comparable bank scores)
- Voice-leading is realized during search (Realizer) so candidates are scored
  on how they actually voice, not just their numerals
- Loop closure pass on slot 0 against slot 3 (same as live playback)

## Quality metrics (§71, internal)

`QualityMetrics::evaluateWithPlan` blends: functional coherence, tension arc,
cadence (style-weighted), loop strength (grammar + bass return), voice-leading
quality, memorability (repetition structure + top-line pitch-class reuse),
vocal space (mid-slot extension density), hook strength (top-line stepwise
ratio), desirable surprise (non-obvious valid moves; zero-weight penalized),
style authenticity. Raw scores are never shown in the UI (§71).

GOLD admission: `total ≥ 0.62`, `functionalCoherence ≥ 0.45`,
`loopStrength ≥ 0.45` on the realized plan.

## Style grammars (M6)

Six styles with degree vocabulary + transition matrix + tension arc +
cadence weight + performance recommendations: Modern R&B, Dark R&B,
Neo-Soul, Emotional, Dark Pop, Trap. Reggaeton is reserved for M7 (needs the
rhythmic engine). FEEL switches the active grammar; PLAY/MORPH/export all
respect it.

## Bank tooling (§100)

```
# regenerate the bank (writes src/engine/bank/GoldBank.inc)
./build/MorphBankTool_artefacts/Release/MorphBankTool --generate --count=50

# inspect an entry's realized plan
./build/MorphBankTool_artefacts/Release/MorphBankTool --audition=42

# per-style counts
./build/MorphBankTool_artefacts/Release/MorphBankTool --stats
```

`tests/bank/BankTests.cpp` re-validates the compiled bank on every test run
(determinism per seed, gold thresholds, per-style uniqueness).

## M7 — Commercial Bank Expansion (planned)

Widen vocabulary: reggaeton/dembow (rhythmic), Latin pop, melodic hip-hop,
emotional trap; deeper per-style content; prioritize songs over count (§101).
No living artist names; no intentional recreation of identifiable songs (§69).
