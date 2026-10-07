# SnairCreator Specification Index

## Project status

v0.0.1 beta implements these specifications (Standalone, VST3, CLAP, AU on macOS). Implementation status and evidence: [spec coverage audit](../../spec-coverage.md). The [CDL Shared Audio Plugin Standard](../../standards/CDL_PLUGIN_BASELINE.md) is the required shared baseline.

## Precedence

1. User-approved product decisions in DECISIONS.md control scope and resolve conflicts.
2. The approved [SnairCreator Design Specification](2026-10-03-snaircreator-design.md) owns product intent, DSP behavior, public controls, platform targets and definition of done.
3. The [VST Implementation Plan](../plans/2026-10-04-snaircreator-vst-implementation.md) owns task order only where it agrees with the design. It must not silently change approved product behavior.
4. Focused specs linked below add concrete interfaces, state rules, edge cases and verification; they do not replace the approved design.

## Focused specification map

| File | Scope |
|---|---|
| [gui-implementation.md](gui-implementation.md) | Main/advanced editor, interaction states, UI-to-parameter/render wiring, accessibility |
| [source-ingestion.md](source-ingestion.md) | File inputs, decode, validation, preprocessing, analysis handoff and error states |
| [render-engine.md](render-engine.md) | Source analysis contract, Snare/Clap render pipeline, determinism, background jobs |
| [plugin-host-state.md](plugin-host-state.md) | VST3/CLAP/AU/Standalone targets, MIDI playback, automation, host state restore |
| [presets-randomization-export.md](presets-randomization-export.md) | Presets, Randomize/Mutate/Undo, exact-render WAV export |
| [verification-build-release.md](verification-build-release.md) | Build, unit/integration/host tests, acceptance gates, packaging evidence |

## Build order

Follow implementation plan Tasks 1–9: native skeleton; parameter model/RNG; source load/analyze; offline render; realtime playback; state/render controller; WAV export; complete GUI; CI/docs/acceptance audit.

## Open reconciliation

The approved design specifies default editor size 1000 × 700 and minimum 760 × 520; the implementation plan's GUI task gives different dimensions. DECISIONS.md resolves this in favor of the approved design. Update the plan before GUI implementation. The design's platform section is authoritative: Windows and macOS first; Linux is optional, AAX out of scope.

No requirement is complete until docs/spec-coverage.md links it to implementation and concrete verification evidence.


## Delivery order

- [Standalone-first specification](standalone-first.md) — first native desktop deliverable; plugin formats are deferred.
