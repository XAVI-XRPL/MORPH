# MORPH — FINAL MASTER BUILD PROMPT V2.0
## Canonical Product + Engineering + UI/UX Contract
## For Kimi Code
## C++20 + JUCE + CMake
## VST3 + AU + Standalone

======================================================================
00 — AUTHORITY OF THIS DOCUMENT
======================================================================

This document is the primary implementation contract for MORPH.

Read it completely before changing architecture or product behavior.

If older MORPH prompts conflict with this document:

THIS DOCUMENT WINS.

Supporting specifications may expand implementation details, but they
must not redefine the core behavior or canonical UI/UX described here.

The supplied MORPH UI reference image is the canonical visual target.

Treat that reference as locked.

Do not redesign the primary interface.

Do not "improve" its layout by replacing it with another design system.

Do not convert it into a generic VST interface.

Do not simplify away its physical character.

Do not add major controls to the main surface without explicit approval.

======================================================================
01 — PRODUCT
======================================================================

Product name:

MORPH


Category:

Generative MIDI Harmony & Songwriting Instrument


Core promise:

PRESS ONE NOTE.
HEAR A COMPLETE MUSICAL CHORD.
PLAY IT IN DIFFERENT WAYS.
BUILD A PROGRESSION.
MORPH IT.
LOCK WHAT YOU LOVE.
BUILD SONG SECTIONS.
DRAG THE MIDI INTO YOUR DAW.


MORPH should feel like:

a sophisticated keyboard player
+
a harmony co-writer
+
a songwriting instrument
+
a premium physical musical object.


It should NOT feel like:

a theory utility
a random chord generator
a DAW
a piano roll
a generic MIDI plugin
a synth
a chatbot
a preset browser with a keyboard attached.

======================================================================
02 — CANONICAL MUSICAL HIERARCHY
======================================================================

The following hierarchy is locked:

INPUT NOTE
    ↓
TRIGGER INTERPRETATION
    ↓
HARMONIC FUNCTION
    ↓
CHORD IDENTITY
    ↓
VOICING
    ↓
PERFORMANCE
    ↓
MIDI EVENTS
    ↓
PLAYBACK STATE
    ↓
VISUALIZATION
    ↓
PROGRESSION
    ↓
SONG SECTION
    ↓
SONG KIT

Do not collapse these layers.

Especially:

CHORD != VOICING

VOICING != PERFORMANCE

PERFORMANCE != PROGRESSION

PROGRESSION != SONG SECTION

======================================================================
03 — THE DEFINING MORPH INTERACTION
======================================================================

The user presses ONE MIDI note.

Example:

Key:
C Minor

Style:
Modern R&B

Input:
C3

MORPH interprets:

Scale Degree:
1

Function:
i

Chord:
Cm9

MORPH realizes an intelligent voicing.

Example:

C2
G2
Bb3
D4
Eb4

MORPH then performs that voicing according to the current
Performance Mode.

The user does not need to manually construct the chord.

======================================================================
04 — PERFORMANCE MODES
======================================================================

The same chord and voicing can be heard in different ways.

V1 REQUIRED:

TOGETHER

STRUM UP

STRUM DOWN

Architecture must support later:

PULSE

PATTERN

ARP

Changing Performance Mode must NOT automatically regenerate:

the chord
the voicing
the progression
the Song Kit.

======================================================================
05 — DEFAULT PERFORMANCE
======================================================================

Default:

TOGETHER

When STRUM is selected, expose contextual parameters such as:

Direction
Spread
Feel

Do not permanently place multiple strum controls on the main surface.

======================================================================
06 — CANONICAL UI REFERENCE
======================================================================

The supplied MORPH UI reference image is now the visual source of truth.

Do not treat it as loose inspiration.

Treat it as the target design system.

======================================================================
07 — UI DESIGN LANGUAGE
======================================================================

PREMIUM PHYSICAL DIGITAL INSTRUMENT.

warm bone / cream polymer, soft molded enclosure, subtle metallic/ceramic
control surfaces, recessed information areas, physical knobs, physical keys,
physical buttons, controlled internal illumination.

Skeuomorphism for touchability + subtle neumorphic depth for enclosure
hierarchy. Avoid: glassmorphism, flat SaaS cards, generic web UI, neon
gaming aesthetics, excessive gradients, fake vintage wear, fake screws,
wood grain, chrome overload, busy labels, decorative technical readouts.

======================================================================
08–09 — CANONICAL CHASSIS / MAIN LAYOUT
======================================================================

One physical chassis: large rounded outer corners, warm off-white/bone
material, soft external shadow, subtle inner perimeter, layered molded edge,
gentle dimensional bevel, physical visual weight.

Canonical logical target: 1440 × 900.

Primary regions: HEADER, LEFT CONTROL BANK, RADIAL HARMONIC FIELD,
RIGHT CONTROL BANK, ACTION ROW, KEYBOARD.

The Radial Harmonic Field is the visual center of gravity.

======================================================================
10–13 — HEADER CONTROLS
======================================================================

LEFT: MORPH wordmark + small warm orange status dot + descriptor
("GENERATIVE / HARMONY / FOR MODERN MUSIC").

CENTER-LEFT: KEY / SCALE segmented pill ([ < | KEY / SCALE | > ]) —
arrows step, center opens selection; never a permanent large dropdown.

CENTER: FEEL pill (e.g. "Modern R&B") — opens deeper style selection.

CENTER-RIGHT: PERFORMANCE pill — TOGETHER (default) / STRUM ↑ / STRUM ↓.
Answers HOW, never WHAT.

RIGHT: MORE pill + minimal brand/status motif.

======================================================================
14–21 — CONTROL BANKS / MACROS
======================================================================

LEFT (TONE & MOVEMENT): COLOR (DARK↔BRIGHT: harmonic brightness),
MOTION (STILL↔MOVING: performance/strum movement; never overrides explicit
Performance selection), MORPH (FAMILIAR↔UNEXPECTED: signature creative
macro — more adventurous musical logic, never randomness).

RIGHT (SPACE & OUTPUT): SPACE (CLOSE↔OPEN: voicing openness — NOT reverb),
TEXTURE (CLEAN↔TEXTURED: performance nuance), OUTPUT (MIDI performance
intensity: velocity baseline/contour/accents — NOT audio volume).

======================================================================
22–28 — RADIAL HARMONIC FIELD
======================================================================

Dark recessed circular field, warm physical outer ring, subtle radial
guides, concentric orbit lines, crosshair, four cardinal progression
positions, center glowing Live Orb, colored harmonic arcs, minimal
functional text, physical colored chord nodes/pucks.

CENTER = what is sounding now. OUTER PUCKS = progression memory
(slot 2 top, slot 1 left, slot 3 right, slot 4 bottom). PATHS = real
voice/harmony relationships. LIGHT = activity/harmonic state.

Semantic colors: BASS blue, INNER orange, TOP yellow, TENSION violet,
INPUT green only if useful.

======================================================================
24 — CENTER LIVE ORB
======================================================================

Stopped: tonal center with subtle warm glow. Sounding: active realized
chord (e.g. "Cm9 / i"), more luminous while active. During STRUM the orb
represents the complete chord identity from the beginning while intensity
may build as voices arrive.

======================================================================
26 — LIVE CHORD VS STORED CHORD
======================================================================

Playing a live chord does NOT overwrite progression memory. Match →
highlight that puck. No match → center only. Commit via explicit action.

======================================================================
29–34 — ACTION ROW / KEYBOARD
======================================================================

LEFT: SAVE UNDO REDO. CENTER: MORPH PLAY (emphasized). RIGHT: EXPLORE
MIDI MORE. Physical rectangular buttons: rounded corners, subtle bevel,
soft shadow, clear pressed state, minimal iconography.

MORPH action = musical sibling (preserve key/style/feel/bars/locks/DNA;
change unlocked harmony/voicing/bass/top/rhythm/performance nuance).

PLAY = progression playback using the SAME engines as live performance.
EXPLORE = deeper functionality (Song Kit, presets, capture). MIDI =
drag/export reproducing actual playback.

Keyboard: permanently visible live truth surface; warm physical white keys,
dark dimensional black keys, soft inset bed, subtle bevel, internal
illumination. Never flat rectangles.

======================================================================
35–42 — TRUTH PIPELINE INVARIANTS
======================================================================

Keyboard geometry generated mathematically from MIDI (naturals
{0,2,4,5,7,9,11}, accidentals {1,3,6,8,10}, groups 2+3, never E/F or B/C) —
unit tested.

Exact MIDI lighting: light exact generated pitches, not pitch classes.
Semantic lighting by VoiceRole. TOGETHER: all illuminate together.
STRUM: keys light in actual attack order — never the intended chord early.

generatedChordNotes != currentlySoundingNotes (both exist; equivalent in
Together, differ mid-strum). One authoritative MusicalPlaybackState.
Core truth invariant (hard, automated): actual sounding MIDI ==
currentlySoundingNotes == keyboard-lit notes.

======================================================================
43–68 — ENGINE / MIDI / LIFECYCLE
======================================================================

Pipeline: MIDI INPUT → TriggerInterpreter → KeyContext → HarmonyEngine →
Chord Selection → VoicingEngine → BassEngine → TopVoiceEngine →
ChordRealization → PerformanceEngine → MidiScheduler →
MusicalPlaybackState → MIDI OUTPUT + RADIAL FIELD + KEYBOARD.

Strongly typed musical state (PitchClass, MidiPitch, Interval, ScaleDegree,
ScaleDefinition, KeyContext, ChordQuality, ChordExtension, ChordSymbol,
ChordFormula, RomanFunction, ChordCandidate, ChordRealization, VoiceRole,
Voice, Voicing, ProgressionSlot, Progression, StyleProfile,
PerformanceMode, PerformanceProfile, StrumProfile, StrumDirection,
StrumCurve, StrumVelocityShape, ScheduledNote, ScheduledMidiEvent,
BassProfile, TopLineProfile, SongDNA, PerformanceDNA, SongSection, SongKit).
Never strings as primary musical state.

Trigger modes: SCALE_DEGREE (default; V1), ROOT, SMART. Chromatic input:
STYLE_COLOR — stylistically justified borrowing only; never arbitrary
chromatic chords.

Voicing: candidate generation + scoring (movement, common tones, leaps,
crossing, register, spacing, low-end cleanliness, top-line quality, bass
quality, style). Low-end cleanliness is a hard requirement: below MIDI 48
penalize clusters; below MIDI 36 strongly favor single bass/octave/
fifth/tenth.

Bass is melodic (BassEngine: ROOT/FLOW/WALK/PEDAL/BOUNCE/GOSPEL/HOUSE/
CINEMATIC). Top voice is melodic (TopVoiceEngine: contour, motif,
stepwise, controlled leaps, destination, climax, resolution). Never
highestChordTone().

PerformanceEngine: attack order/timing/velocity/duration/articulation/
retrigger — never harmony. TOGETHER profiles TIGHT/SOFT/HUMAN/WIDE.
STRUM_UP/STRUM_DOWN mandatory (architecture for more), StrumProfile with
direction/spreadMs/curve/velocityShape/bassPolicy/topVoicePolicy/
randomTiming/randomVelocity/noteLengthPolicy/retriggerPolicy/tempoSync/
styleInfluence. Spread = first-to-last attack time. Curves LINEAR/EASE_IN/
EASE_OUT/HUMAN (human = musician-like, not jitter). BassStrumPolicy
WITH_STRUM/ANCHOR_FIRST/ANCHOR_SIMULTANEOUS/DELAYED/EXCLUDED.
TopVoicePerformancePolicy NORMAL/ARRIVE_LAST/ARRIVE_FIRST/ACCENT/HOLD.
Curated presets only: TIGHT/SOFT/HUMAN; SOFT/FAST/LAZY UP+DOWN, INTIMATE,
WIDE.

MidiScheduler: realtime-safe, sample offsets, correct across blocks; never
sleep()/UI timers/thread blocking. Release-before-strum-end default
CANCEL_PENDING_ATTACKS. Retrigger: cancel obsolete pending, release
irrelevant, preserve common tones, no duplicate lifecycle errors. Sustain
CC64 everywhere; dedicated stuck-note tests.

Progression memory: 4 slots storing musical INTENT (RomanFunction,
ChordColor, VoicingIntent, BassIntent, TopLineIntent, RhythmIntent,
PerformanceIntent); playback dynamically realizes. Live override while
sequencer plays: hold suppresses sequenced output, release resumes at a
safe quantization boundary. Never stuck notes.

MORPH deterministic for same seed+parameters. Locking: slot/harmony/
voicing/bass/top/rhythm/performance/section — locked never mutates.
Undo/redo checkpoints for destructive composition actions, deterministic
restore.

======================================================================
69–80 — BANK / QUALITY / GENERATION / SONG KIT / EXPORT / CAPTURE
======================================================================

Progression bank stored functionally; no living artist names; no
intentional recreation of identifiable songs. Commercial = memorable,
singable, vocal space, hook potential, loop strength, bass identity,
top-line opportunity, section payoff. Internal quality metrics never
exposed raw. Sequence-aware generation (beam search, width 32, branch 12).
Song Kit: sections above progression memory — different sections, same song
(SongDNA + PerformanceDNA + section roles); lives under EXPLORE, never
redesigns main UX. MIDI export reproduces actual performance (ALL/CHORDS/
BASS/TOP/PERFORMANCE; section/song/stems later). Capture: realtime-safe
rolling 60 s history, hidden from main surface.

======================================================================
81 — REALTIME SAFETY
======================================================================

Never in the realtime callback: heap allocation, mutex locking, filesystem,
networking, expensive generation, large dynamic programming, UI calls,
heavy logging. Use fixed-capacity queues, preallocated structures,
immutable snapshots, sample offsets, bounded work.

======================================================================
82–86 — UI ARCHITECTURE / MATERIAL / SHADOW / TYPE / ANIMATION
======================================================================

Components: MorphEditor, MorphChassis, MorphHeader, KeyScaleControl,
FeelControl, PerformanceControl, MorphKnob, MorphButton, RadialField,
RadialGuideLayer, RadialChordPuck, RadialCenterOrb, RadialPathLayer,
MorphActionButton, PlayButton, ExploreButton, MidiDragControl,
MorphKeyboard, MorphWhiteKey, MorphBlackKey, KeyLightLayer, ToastLayer,
TooltipLayer, ContextPopover, SongKitPanel. Visual components never own
musical logic.

Reusable material primitives; no raster screenshots as the interface;
responsive/vector-capable. Shadows communicate physical hierarchy; molded,
not floating. Clean premium geometric sans; small uppercase tracked labels;
larger clear values. Animation explains musical behavior: ~90 ms feedback,
~140 ms control, ~220 ms state change, tempo-aware musical; key note-on
ramp 25–45 ms, note-off 100–180 ms; no random particles or decorative
pulsing.

======================================================================
87–94 — STRUCTURE / DOCS / TESTING
======================================================================

Repo: src/{engine,midi,state,plugin,ui}, tests/{unit,theory,harmony,
voicing,performance,midi,keyboard,progression,songkit,integration,golden},
resources, docs, tools.

Required docs: README.md, MASTER_BUILD_PROMPT.md, PRODUCT_BEHAVIOR.md,
ARCHITECTURE.md, MUSIC_ENGINE.md, PERFORMANCE_ENGINE.md,
PROGRESSION_BANK.md, SONG_KIT.md, UI_SYSTEM.md, RADIAL_FIELD.md,
MIDI_ENGINE.md, TESTING.md, ROADMAP.md, DECISIONS.md.

Testing: theory; keyboard geometry; performance (together tolerance, strum
orders, spread, early-release cancel, retrigger, sustain, export timing);
playback truth (MIDI == state == keyboard; active chord == orb; slot ==
puck when matched); morph/lock determinism; song kit.

======================================================================
95–104 — MILESTONES
======================================================================

M1 PLAYABLE FOUNDATION. M2 PERFORMANCE (strum up/down, spread, curves,
velocity shaping, policies, lifecycle, sustain, retrigger, sequential
lighting, performance-aware export). M3 MUSICAL QUALITY (candidate
voicings, voice-leading optimization, BassEngine, TopVoiceEngine, low-end
cleanliness, StyleProfile, four-chord progression playback).
M4 CANONICAL UI (reproduce the reference; do not redesign).
M5 CREATIVE WORKFLOW (morph macro/action, lock, undo/redo, save, explore,
MIDI drag/export). M6 PROGRESSION INTELLIGENCE (functional generation,
beam search, style profiles, 300 GOLD seeds). M7 COMMERCIAL BANK.
M8 ADVANCED PERFORMANCE (pulse/pattern/arp only after strum is excellent).
M9 SONG KIT (10 exceptional GOLD kits, then 100, then 250+).
M10 SONG WORKFLOW (build song/section workflows; main UI unchanged).

======================================================================
105–106 — GOLDEN TESTS
======================================================================

Golden musical scenario: C minor, Modern R&B, input C3 → function i,
chord family Cm9, example realization C2 G2 Bb3 D4 Eb4 (bass C2, top Eb4).
TOGETHER: all attack together. STRUM UP: C2 G2 Bb3 D4 Eb4. STRUM DOWN:
Eb4 D4 Bb3 G2 C2. Identity/roles/visual truth/performance must stay
correct even as the VoicingEngine evolves.

First gold progression family: i9 → bVImaj9 → iv9 → V7alt/V7sus
(e.g. Cm9 → Abmaj9 → Fm9/Fm11 → G7sus/G7alt) — an initial test family,
never a hardcoded engine.

======================================================================
107–115 — OPERATING RULES / DO-NOT-CHANGE / FIDELITY / NORTH STAR
======================================================================

Compile frequently, run tests frequently, keep architecture clean, document
decisions. Completion means behavior works — not that a class exists.

Do not change: one note → one chord; orb = current harmony; pucks =
progression memory; keyboard = exact MIDI output; bass blue / inner orange /
top yellow; Together and Strum are separate performances of the same
harmony/voicing; live playing never overwrites memory; locked never mutates;
export reproduces playback; voice leading first-class; bass melodic; top
voice melodic; progression generation contextual not random; Song Kit
subordinate to Radial Field; advanced complexity hidden until requested;
the supplied physical UI is canonical; the dark circular Radial Harmonic
Field remains the visual identity; no main-UI redesign without approval.

UI fidelity: match the actual visual hierarchy — warm rounded chassis,
wordmark upper left, segmented header controls, TOGETHER selector, left
three-knob bank, right three-knob bank, large dark circular radial center,
four cardinal colored nodes, luminous warm center, concentric guides,
restrained colored arcs, physical action row, MORPH+PLAY emphasized
centrally, full-width inset physical keyboard, internal blue/orange/yellow
illumination, generous controlled spacing, physical depth.

Responsive: preserve proportions 1280×800 – 1680×1050, minimum 1080×675;
never remove keyboard/radial field or relocate major controls.

Definition of done: MUSIC, INTERACTION, VISUALIZATION agree — what the
engine thinks, what the user hears, what the user sees, and what gets
exported are the same musical truth.

PRESS. HEAR. FEEL. MORPH. KEEP. PLAY. EXPORT.
