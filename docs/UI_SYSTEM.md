# MORPH — UI_SYSTEM

Canonical language: **premium physical digital instrument** (§7) — warm bone
polymer chassis, molded depth, ceramic/metal control surfaces, controlled
internal illumination. All vector-drawn (§83); no raster UI. The binding
token/layout spec extracted from the locked reference is
`docs/UI_REFERENCE_SPEC.md`.

## Design canvas & scaling (§111)

Fixed 1440×900 design space in `MorphChassis`; `PluginEditor` applies a
uniform `AffineTransform::scale` centered with locked aspect. Resize limits
1080×675 … 1680×1050. Children are laid out in design coordinates once;
fonts inside components are size-relative where required.

## Design system (`src/ui/design_system/`)

- `MorphTheme` — palette tokens (chassis/radial/semantic/text/control colors)
  and font helpers. Semantic voice colors are locked (§27): bass blue
  `#5E86C4`, inner orange `#E8873F`, top yellow `#F2C94C`, tension violet.
- `MaterialPainters` (namespace `materials`) — drawChassisShadow/Surface,
  drawRecessedWell, drawRaisedControl, drawKnob (metal ring + molded body +
  ceramic cap + pointer, 0.5 = up), drawRadialSurface (dark disc + warm rim +
  inner shadow), drawLed, drawEngravedLabel (tracked small caps), drawPuck
  (sphere + glow), drawOrb (layered warm glow; dim at rest, luminous when
  active).
- `MorphLookAndFeel` — rotary slider + button rendering, popup styling.

## Components (`src/ui/`)

- `MorphChassis` — enclosure, bank wells, headings with hairlines, knob
  furniture (labels/sublabels/status dots), "MIDI HARMONY ENGINE" motif,
  footer ("ALWAYS A NEXT CHORD" / "MORPH").
- `header/` — `MorphHeader` (wordmark + status dot + descriptor; motif),
  `HeaderControls`: `SegmentedPillControl` (KEY/SCALE, FEEL — arrows + center
  click), `PerformancePillControl` (TOGETHER ▾), `MorePillControl`.
- `controls/ActionRow` — SAVE UNDO REDO | **MORPH** (accent) **PLAY** |
  EXPLORE MIDI MORE; physical buttons with drawn glyphs, pressed states,
  status dots on the two signature actions; REDO dimmed at rest.
- `radial/RadialField` — see RADIAL_FIELD.md.
- `keyboard/` — `KeyboardGeometry` (pure, unit-tested, §35) +
  `MorphKeyboard` (inset bed, beveled white/black keys, exact-pitch semantic
  lighting with glow spill, §34/§36/§37; click keys to play).

## Wiring discipline

Visual components never own musical logic (§43). Everything visual reads the
`MusicalPlaybackState` snapshot (30 Hz timer) or edits APVTS parameters, which
the processor mirrors into engine atomics on the audio thread.

## Animation (§86)

State changes ease at ~90–220 ms (orb intensity ease, transition-arc fade
≈700 ms cap); keyboard illumination follows scheduled MIDI exactly (ramp is
the attack itself); no decorative pulsing or particles.
