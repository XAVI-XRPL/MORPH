# MORPH — RADIAL_FIELD

The Radial Harmonic Field is the visual identity (§22) and is never
decorative: every element maps to musical state (§23).

## Layout (`src/ui/radial/RadialField.cpp`)

- Dark recessed disc with warm physical rim and inner shadow
  (`materials::drawRadialSurface`); "RADIAL HARMONIC FIELD" micro-label rides
  the top inner rim.
- Crosshair + three concentric dotted orbit lines + center tick.
- **Center Live Orb** (§24): dim warm tonal-center glow when idle; represents
  the active realized chord while sounding — identity text (roman above,
  chord symbol below) and luminosity follow `MusicalPlaybackState`. During a
  strum the orb carries the full chord identity from the first attack.
- **Progression pucks** (§25): four cardinal physical spheres — slot 1 left,
  slot 2 top, slot 3 right, slot 4 bottom; Roman function above, chord symbol
  below. Live-chord match highlights the puck without overwriting memory
  (§26); the sequencer's active slot glows (§64).
- **Voice arcs** (§28): one rim arc per sounding voice, angle mapped from
  pitch (low → lower-left, high → upper-right), colored by VoiceRole
  (bass blue / inner orange / top yellow). Because arcs read
  `currentlySoundingNotes`, they arrive in real attack order during strums —
  the signature strum visualization (§39).
- **Transition arc**: when the sequencer advances, a brief controlled arc
  bows from the previous slot puck to the current one (fade ≈700 ms).

## Truth source

The field reads only the `MusicalPlaybackState` snapshot (plus static
progression display data). The hard invariant — actual MIDI ==
`currentlySoundingNotes` == keyboard-lit notes — is test-verified
(`truth.*` suites), and the field shares that same state, so what the field
shows is what sounds.
