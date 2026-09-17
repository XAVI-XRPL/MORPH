# MORPH — MUSIC_ENGINE

Strongly-typed musical state throughout (§44); never strings as state.

## Theory (`src/engine/theory/`)

- `PitchClass` (0–11, C=0, wraps), `MidiPitch` (0–127), `Interval` (semitones),
  `ScaleDegree` (1–7), `ScaleDefinition` (naturalMinor {0,2,3,5,7,8,10}, major),
  `KeyContext` (tonic + scale; degree→pitch-class, membership, spelling pref).
- `ChordQuality` (major/minor/diminished/halfDiminished/augmented/sus2/sus4/
  dominant), `ChordExtension` bitmask (b7, maj7, 9, b9, #9, 11, #11, b13, 13),
  `ChordFormula::make` compiles quality+extensions to semitone offsets
  (dominant and halfDiminished imply b7).
- `RomanFunction`: degree on the MAJOR diatonic lattice + accidental — so
  natural-minor degrees 3/6/7 render as bIII/bVI/bVII (§45 convention).

## TriggerInterpreter (§45, §46)

SCALE_DEGREE (default): input pitch class → scale degree of the current key.
Chromatic input (STYLE_COLOR): snaps to the nearest valid degree, ties resolve
downward; never yields arbitrary chromatic chords. ROOT/SMART: architecture
reserved.

## HarmonyEngine (§47)

Modern R&B natural-minor functional map (degree → quality + extensions):

| Degree | Roman | Chord (C minor) |
|---|---|---|
| 1 | i | Cm9 |
| 2 | iiø | Dø7 |
| 3 | bIII | Ebmaj9 |
| 4 | iv | Fm9 |
| 5 | V | G7sus |
| 6 | bVI | Abmaj9 |
| 7 | bVII | Bb9 |

COLOR (brightness): <0.33 strips to plain sevenths; 0.33–0.66 canonical ninth
vocabulary; >0.66 adds the 11th on minor functions. Stylistically contained
(§15). Deeper function reasoning (secondary dominants, modal interchange) is
M6 territory.

## VoicingEngine (§48, §49)

Register-band recipe (DECISIONS D9), deterministic and stateless
(`realize(candidate, key, style, previous, openness, variation)`):

- **Bass**: root in octave 2 ([36..47]); single bass only (§49).
- **Low inner**: 5th (or b5) a fifth above the bass.
- **Mid inner**: 7th placed in octave 3, kept above the low inner.
- **High inner**: 9th (then 11th) in octave 4.
- **Top**: 3rd (4th for sus) above everything — the melodic identity.
- **Top-voice continuity**: octave-folded toward the previous top when musically
  legal (sequence coherence; full candidate scoring is M3).
- **SPACE openness**: >0.66 lifts 9th+top an octave; <0.33 tightens the top.
- **Morph variation** (interim MORPH action, D13): deterministic siblings —
  v1 top +12, v2 upper structure +12, v3 seventh drops an octave.

Golden verification: degree 1 in C minor → C2 G2 Bb3 D4 Eb4 exactly
(`voicing.goldenCm9Realization`).

## Progression (`src/engine/progression/`)

Four slots of musical intent (scale degrees), default gold family (§106):
1, 6, 4, 5 → Cm9 → Abmaj9 → Fm9 → G7sus. Playback realizes dynamically
through the same pipeline (§64).
