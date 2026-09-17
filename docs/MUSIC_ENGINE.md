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

M3: candidate generation + voice-leading scoring (D15). Stateless
(`realize(candidate, key, style, VoicingContext, variation)`).

- **No context** → canonical register-band recipe (golden Cm9 preserved):
  bass root octave 2, 5th above bass, 7th in octave 3, 9th/11th in octave 4,
  3rd/4th on top.
- **With context** → candidates over (bass × top): bass from **BassEngine**
  (root/flow/pedal/bounce, anti-parking flow, register [36..47]), top from
  **TopVoiceEngine** (melody register [60..76], stepwise + repetition +
  controlled leaps + contour memory). Inner voices fill the register bands.
- **Scoring** (§48): total voice movement (nearest-neighbor), common-tone
  bonus, large-leap penalty, register and spacing discipline, bass quality
  (stepwise + root bonus), top-line quality (weighted ×2). Crossing and
  low-end violations are structurally impossible by construction (§49).
- **Voice-leading memory** resets on full silence (D16) — a new phrase
  starts canonical.
- **SPACE openness** and **morph variation** apply as before (D13).

## Sequence plan (M3, §48/§64)

`EngineHost::buildProgressionPlan` realizes the 4 slots forward with
threaded voice-leading memory, then loop-closes slot 0 against slot 3.
Cached per settings signature; the sequencer and MIDI export render from
identical plans (export == playback). Gold family plan:

| Slot | Chord | Bass | Top |
|---|---|---|---|
| 1 | Cm9 | C2 | D4 |
| 2 | Abmaj9 | Eb2 | Eb4 |
| 3 | Fm9 | F2 | Eb4 |
| 4 | G7sus | G2 | D4 |

Golden verification: degree 1 in C minor → C2 G2 Bb3 D4 Eb4 exactly
(`voicing.goldenCm9Realization`).

## Progression (`src/engine/progression/`)

Four slots of musical intent (scale degrees), default gold family (§106):
1, 6, 4, 5 → Cm9 → Abmaj9 → Fm9 → G7sus. Playback realizes dynamically
through the same pipeline (§64).
