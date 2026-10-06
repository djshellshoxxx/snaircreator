# SnairCreator GUI Implementation Specification

## Scope and source of truth

This document makes the approved design's GUI implementable. The design specification remains the authority for product appearance, controls, macro meanings and editor dimensions. The editor hosts an instrument plug-in UI; the Standalone build uses the same component tree with a standalone audio/MIDI shell.

## Layout contract

Use the approved design dimensions: default editor 1000 × 700, resizable minimum 760 × 520. If host constraints prevent this, preserve aspect and usability; do not introduce a second arbitrary layout. Main order:
1. Header: product identity, version, About/menu.
2. Source panel: drop target, Load Source, filename/duration, waveform, analysis/render status.
3. Mode selector: Snare or Clap.
4. Six macros: Punch, Snap, Body, Texture, Dirt, Size.
5. Source Character control.
6. Actions: Preview, Randomize, Mutate, Undo, Reset.
7. Bottom status/actions: MIDI activity, render state, Export WAV, Save/Load Preset.
8. Advanced panel: only visible on user request; never required for the common workflow.

The waveform is display-only in the first release. Mark strongest transient and selected attack region where available; do not imply destructive editing.

## UI states

| State | Source area | Controls | Preview/export |
|---|---|---|---|
| Empty | Load/drop guidance | Mode/macros at defaults; generation actions disabled | Disabled |
| Loading | Filename and decode progress | Disable conflicting source/mode actions; allow cancel | Disabled |
| Analyzing | Waveform/progress and analysis summary | Existing sound may continue playing | Existing hit preview allowed if present |
| Ready | Source and analysis shown | All valid controls enabled | Current rendered hit |
| Rendering | Progress/status; old hit remains active | Coalesce parameter changes; do not queue obsolete renders | Preview/MIDI uses old hit until atomic swap |
| Source missing after restore | Missing source notice and fingerprint | Existing generated hit remains usable; source-dependent reanalysis disabled | Restored hit can play/export |
| Error | Actionable message and preserved previous state | Disable only actions dependent on failed operation | Existing valid render remains playable |

Progress and failure must remain visible until resolved. A parameter edit that triggers rendering never blocks audio playback.

## Interaction wiring

- Mode and macro changes update stable host parameter IDs and visible values.
- Source Character remains a user-facing macro with multi-stage DSP influence; do not wire it as a dry/wet fader.
- Advanced controls update the same parameter model as macros; both stay synchronized.
- Parameter changes requiring regeneration enqueue/coalesce background work. Old rendered buffer remains current until a valid new render is ready.
- Preview and MIDI playback use the identical immutable RenderedHit object.
- Randomize, Mutate, Undo and Reset update parameter/seed state through the controller; they may not call DSP directly from UI.
- Export opens a chooser and passes the current immutable RenderedHit and export options to the exporter.
- File load/drop hands off a path to SourceLoader; UI never decodes the file itself.
- Double-click reset, numeric entry, tooltips and mouse-wheel behavior follow approved theme/interaction rules. Wheel changes must be fine and not trigger large accidental jumps.
- If a render fails, retain the last valid hit and show that it does not match current visible pending settings until a new render succeeds.

## Accessibility and input

Visible labels, keyboard focus, numeric editing for advanced controls, reset-to-default, useful tooltips, high contrast, and sufficient contrast are required. MIDI activity and render status have accessible text. All controls are keyboard operable where practical. No status depends on color or animation alone. Avoid flashing meters and respect host/OS scaling.

## Theme adaptation

Use the approved SnairCreator design palette and styling direction; do not substitute the generic Booth Check palette. Shared theme controls (knob/slider interaction, typography, spacing, tooltips, icon/version treatment) may be reused only if they match the approved product design. Macro controls need numeric readouts and clear value range. Do not add controls not backed by an approved parameter or action.

## Acceptance criteria

- Editor instantiates in VST3, CLAP, AU where built, and Standalone.
- Empty/loading/analyzing/ready/rendering/error/source-missing states behave as tabled.
- Macro and Advanced controls stay synchronized with stable host parameters.
- Rendered sound remains active during a new render and swaps without click or invalid memory.
- Preview, MIDI and WAV export use the same current hit.
- Accessibility, scaling, keyboard and mouse interaction pass the approved design's QA checks.
