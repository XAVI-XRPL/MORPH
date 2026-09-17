# MORPH — Canonical UI Reference Spec (Milestone 4)

Extracted from the locked reference image. Treat as binding. Do not redesign.
Logical canvas: **1440 × 900** (preferred range 1280×800 – 1680×1050,
minimum 1080×675 — scale proportionally, §111).

## Palette (approximate hex from the reference)

| Token | Value | Use |
|---|---|---|
| studio backdrop | #EDEBE6 | area outside chassis |
| chassis top | #F3F0EA | main body gradient start |
| chassis bottom | #DCD8D0 | main body gradient end |
| chassis rim light | #FBFAF6 | outer edge highlight |
| chassis rim shade | #B7B1A6 | outer edge shade |
| panel inset | #E3DFD7 | recessed banks / keyboard bed |
| hairline | #C6C1B7 | engraved separators |
| radial deep | #211E1A | radial field core |
| radial ring | #2E2A25 | radial field rim |
| radial guide | #4A443C | concentric guides |
| orb core | #FFD9A0 | live orb highlight |
| orb glow | #FF9E3D | live orb body |
| bass blue | #5E86C4 / glow #6FA3E8 | BASS (locked §27) |
| inner orange | #E8873F / glow #F0A05C | INNER (locked) |
| top yellow | #F2C94C / glow #FFD966 | TOP (locked) |
| tension violet | #9B7FD4 | altered color only |
| text primary | #3B3733 | dark warm brown-gray |
| text secondary | #8A847C | engraved micro-labels |
| accent orange | #E8783C | MORPH action emphasis |
| play green | #4DA35E | PLAY glyph |

## Layout (1440×900, margins ~28)

1. **Header** (y 24–132)
   - Left: `MORPH` wordmark (bold geometric sans, ~34px, tracking +6%),
     small warm-orange status dot, then descriptor stacked
     `GENERATIVE / HARMONY / FOR MODERN MUSIC` (7–8px caps, tracking +18%).
   - KEY / SCALE segmented pill: `[ < ] [ KEY / SCALE · C Minor ] [ > ]`
     (~230×64). Label caps 9px over value 17px.
   - FEEL pill: same geometry, `FEEL · Modern R&B`.
   - PERFORMANCE pill (~150×64): `TOGETHER ▾` — dropdown, no arrows.
   - MORE pill (~120×64): `MORE ···`.
   - Far right: three small status dots (orange, two gray) +
     `IDEAS FLOW FURTHER` micro-caps.
2. **Control banks** (y ~150–560, w ~430 each)
   - Heading caps 13px centered with hairlines: `TONE & MOVEMENT` (left),
     `SPACE & OUTPUT` (right).
   - Three knobs each, ~110px diameter, colored caps:
     COLOR blue, MOTION orange, MORPH cream · SPACE taupe, TEXTURE tan,
     OUTPUT orange. Small status dot above each knob (MORPH dot lit orange).
   - Knob label 15px caps + sublabel 10px caps (MOOD/RHYTHM/VARIATION,
     AMBIENCE/CHARACTER/LEVEL).
   - Right bank footer: `MIDI HARMONY ENGINE` + hand-drawn waveform motif.
3. **Radial Harmonic Field** (center, ~Ø 500)
   - Dark recessed disc, warm physical rim, inner shadow.
   - `RADIAL HARMONIC FIELD` micro-caps around the top inner edge.
   - Concentric dotted/solid guides, crosshair, four cardinal pucks:
     slot1 left, slot2 top, slot3 right, slot4 bottom. Roman above puck,
     chord symbol below. Pucks are physical colored spheres.
   - Center Live Orb: warm luminous, intensity follows sounding state.
   - Sparse semantic arcs along the rim (blue/orange/yellow).
4. **Action row** (y ~590–670, h ~56)
   - Left: SAVE · UNDO · REDO (icons + caps). Center: MORPH (orange accent,
     waveform glyph, status dot) · PLAY (green triangle, status dot).
   - Right: EXPLORE · MIDI · MORE.
   - Physical rectangular buttons, rounded ~14px, soft shadow, pressed state.
5. **Keyboard** (y ~690–860, full width, inset bed)
   - Warm white keys, dark dimensional black keys (h ≈ 58% of white),
     subtle bevels, soft inset bed. Lit keys glow with role color and
     spill onto neighbors.
6. **Footer** (y ~870): left `ALWAYS A NEXT CHORD`, right `MORPH —`,
   micro-caps, engraved.

## Material rules (§7, §83, §84)

- Skeuomorphic touch surfaces + subtle neumorphic enclosure depth.
- Soft outer shadows, subtle inner shadows, small contact shadows,
  gentle bevel highlights. Molded, not floating.
- No glassmorphism, no flat cards, no neon, no fake wear/screws/wood.
- Vector-drawn (no raster screenshots as the interface).

## Typography (§85)

Clean geometric sans (system San Francisco fallback). Labels small
uppercase with generous tracking; values larger and clear. No pixel /
retro / sci-fi / serif fonts.

## Animation (§86)

Fast feedback ~90 ms, control ~140 ms, state change ~220 ms,
key note-on ramp 25–45 ms, note-off 100–180 ms. No decorative pulsing.
