# MORPH — MIDI_ENGINE

`src/midi/` + scheduling core of `EngineHost`.

## MidiScheduler (§59)

- Absolute monotonic sample clock; events carry absolute on/off sample times
  (`ScheduledNote`: pitch, velocity, noteOnSampleOffset, noteOffSampleOffset,
  role).
- Fixed-capacity sorted pending store (256 events), preallocated — no heap,
  no locks, no timers, no blocking (§81). Strums cross block boundaries by
  construction (`midi.eventsFireAcrossBlocks`).
- Groups: every scheduled chord owns a group id for lifecycle operations.
- `releaseGroup` = CANCEL_PENDING_ATTACKS + note-offs for sounding notes
  (sustain-aware). `releasePitch` for surgical per-note release.
- `adoptSounding` moves a sounding pitch into a new group without re-attack —
  the common-tone-preserving retrigger (§61).
- Sustain (CC64): note-offs defer while held; lifting the pedal releases the
  deferred set (§62). `allNotesOff` bypasses sustain (panic) — stuck notes
  are structurally impossible (`midi.noStuckNotesAfterAllNotesOff`).
- `rebuildStateBits` rebuilds `currentlySoundingNotes` / `pendingScheduledNotes`
  from ground truth every block — the state can never drift from emitted MIDI.

## EngineHost orchestration

- Live trigger is monophonic (DECISIONS D8): the latest input owns the live
  chord; a new trigger cancels obsolete pending attacks, releases non-common
  tones, adopts common tones, schedules the new realization (§61).
- Mode change (§112): `requestPerformanceMode` re-performs the SAME
  realization in the new mode (chord/voicing untouched, §109.8).
- Sequencer (PLAY): one chord per bar, host tempo (80 BPM fallback in
  standalone), finite per-bar note lengths, slot advance, live override —
  holding a note suppresses sequenced output; release resumes at the next
  bar boundary (§65).
- Host MIDI and on-screen-keyboard events merge with sample offsets; the UI
  path enters through a lock-free SPSC queue at block start.

## MidiExporter (§79, §33)

- `EngineHost::renderProgressionPerformance()` / `renderCurrentChordPerformance()`
  re-run the same deterministic pipeline with the same seeds as playback;
  `MidiExporter::buildMidiFile` maps samples → ticks (960 PPQ) into a
  type-1 SMF. Export timing == heard scheduling (`midiExport.*` tests).
- UI: MIDI action button → save dialog (`.mid`). Drag-out lands with M5.

## Capture (§80)

Rolling 60 s history is planned (not implemented); it will read the same
scheduled-event stream and stay off the main surface.
