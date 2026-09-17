# MORPH

**Generative MIDI Harmony & Songwriting Instrument**
C++20 · JUCE 8 · CMake · Standalone + VST3 + AU (auval-verified)

> PRESS ONE NOTE. HEAR A COMPLETE MUSICAL CHORD. PLAY IT IN DIFFERENT WAYS.
> BUILD A PROGRESSION. MORPH IT. LOCK WHAT YOU LOVE. BUILD SONG SECTIONS.
> DRAG THE MIDI INTO YOUR DAW.

MORPH turns one played note into an intelligent, voiced, performed chord —
and shows the exact musical truth on a dark Radial Harmonic Field and a
physical illuminated keyboard. The canonical contract is
`docs/MASTER_BUILD_PROMPT.md`; the locked main-screen design is specified in
`docs/UI_REFERENCE_SPEC.md`.

## Status

Milestones **1 (Playable Foundation)**, **2 (Performance)** and
**4 (Canonical UI)** are implemented and tested. See `docs/ROADMAP.md`.

- Press C3 → Modern R&B Cm9 (C2 G2 Bb3 D4 Eb4), bass blue / inner orange /
  top yellow, exactly those keys lit.
- TOGETHER / STRUM ↑ / STRUM ↓ re-perform the same chord without changing
  its identity.
- PLAY sequences the 4-slot progression (i9 → bVImaj9 → iv9 → V7sus) through
  the same engine; live playing overrides and re-joins at the bar boundary.
- MIDI export writes the exact scheduled performance to a `.mid` file.

## Build

```sh
git submodule update --init --depth 1
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 10
# Standalone: build/Morph_artefacts/Release/Standalone/MORPH.app
# VST3:       build/Morph_artefacts/Release/VST3/MORPH.vst3
# AU:         build/Morph_artefacts/Release/AU/MORPH.component
```

## Test

```sh
ctest --test-dir build --output-on-failure
# or: ./build/MorphTests_artefacts/Release/MorphTests
```

42 suites, ~2400 checks: theory, harmony, voicing (golden Cm9), performance
(strum order/spread/curves, policies), MIDI lifecycle (release/retrigger/
sustain, stuck-note safety), export timing, keyboard geometry, and the
core truth invariant (actual MIDI == playback state == keyboard lights,
checked at every block boundary).

## UI snapshots (visual verification)

```sh
cmake --build build --target MorphUISnapshot
./build/MorphUISnapshot_artefacts/Release/MorphUISnapshot.app/Contents/MacOS/MorphUISnapshot /tmp/morph_shots
```

Renders `morph_idle / morph_golden_chord / morph_strum / morph_play.png`.

## Layout

- `src/engine/` — theory, harmony, voicing, performance, progression (UI-free)
- `src/midi/` — realtime MidiScheduler, lifecycle, MidiExporter
- `src/state/` — MusicalPlaybackState + lock-free UI snapshot
- `src/plugin/` — processor, editor, standalone audition synth
- `src/ui/` — design system, header, radial field, keyboard, controls
- `tests/` — zero-dependency harness + suites
- `tools/` — offscreen UI snapshot renderer
- `docs/` — the contract, architecture, and decisions
