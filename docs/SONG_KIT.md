# MORPH — SONG_KIT

**Status: planned (Milestones 9–10). Not implemented yet.**

This document fixes the design intent so the feature lands without touching
the main UX (§73–§78, §104).

## Shape

- Song Kit is a composition layer ABOVE progression memory: INTRO, VERSE,
  PRE_CHORUS, CHORUS, POST_CHORUS, BRIDGE, OUTRO.
- Core principle: DIFFERENT SECTIONS. SAME SONG. — shared key, style,
  SongDNA, bass/top-line vocabulary, voicing language, performance language,
  rhythmic identity, emotional identity.
- Lives under EXPLORE → SONG KIT. Optional compact section strip
  (VERSE | PRE | CHORUS | BRIDGE) loads a section's progression into the same
  Radial Field. No DAW timeline. Main screen unchanged (§74).

## Planned types (§75, §76)

- `SongDNA`: tonalCenter, modeIdentity, brightness, darkness, chromaticism,
  extensionDensity, voiceSmoothness, bassActivity, topLineActivity,
  rhythmicDensity, syncopation, cadenceStrength, harmonicGravity,
  melodicOpenness, vocalSpace, surprise, texture, humanization.
- `PerformanceDNA`: preferredMode, strumDirectionBias, spreadRange,
  velocityShape, humanization, articulation, bassPolicy, topVoicePolicy,
  rhythmicDensity. Sections read like arrangement decisions (verse: soft
  together; pre: soft strum up; chorus: tight together; bridge: wide strum
  down).
- Section roles (§77): verse = space/restraint/loopability; pre = tension/
  expectation; chorus = payoff/hook/lift; post = minimal/memorable; bridge =
  contrast, same identity.

## Generation process (§78)

Establish SongDNA → choose anchor section (usually CHORUS or VERSE) → analyze
anchor → extract motif DNA → generate adjacent candidates → evaluate
transitions → optimize bass/top-line/voicing/performance continuity →
evaluate emotional arc → retain strongest kit. Never independent sections.

Song Kit acceptance tests (§94): locked section survives rebuild; SongDNA
preserved; pre-chorus tension rises; chorus lift occurs; transitions pass
threshold; full kit deterministic; full kit export ordered correctly.
