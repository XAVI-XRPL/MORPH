# MORPH — PROGRESSION_BANK

## Current state (post M2)

Progression memory = 4 slots of musical intent (scale degrees in the current
key/style), never fixed MIDI (§63). Default GOLD family (§106):

```
i9 → bVImaj9 → iv9 → V7sus        (C minor: Cm9 → Abmaj9 → Fm9 → G7sus)
```

- `src/engine/progression/Progression.h` — `ProgressionSlot { ScaleDegree,
  locked }`, `Progression` (4 slots).
- Playback: `EngineHost::scheduleSequencerSlot` realizes each slot through
  Harmony → Voicing (with previous-voicing continuity) → Performance with a
  fixed per-slot seed (`0x51ED270B ^ slot*144665`) so playback and MIDI export
  render identically.
- Live chords never overwrite memory (§26): a live chord matching a stored
  slot highlights the puck; otherwise it shows in the center only.

## Planned (M6–M7, contract §69–§72)

- Functional progression generation with sequence-aware beam search
  (beamWidth 32, branchFactor 12); scoring by functional coherence, tension
  arc, cadence, voice-leading/bass/top-line potential, style authenticity,
  memorability, loop strength, vocal space, desirable surprise, tonal clarity.
- Seed bank starting with **300 GOLD seeds**; validation, deduplication,
  audition tooling before expansion.
- Target styles: Modern R&B, Dark R&B, Alt R&B, Neo-Soul, Trap, Dark Trap,
  Reggaeton, Dark Reggaeton, Dembow, Latin Urban, Afro-Urban, Moody Pop,
  Dark Pop, Commercial Pop, Emotional R&B, Heartbreak, Cinematic.
- No living artist names; no intentional recreation of identifiable songs.
- Quality metrics stay internal (HookStrength, VocalSpaceScore, …).

Bank philosophy (§70): memorable, singable, vocal space, hook potential,
loop strength — "I can write a song to this."
