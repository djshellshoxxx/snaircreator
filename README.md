# SnairCreator

SnairCreator is a JUCE/C++ desktop application that will turn a user-provided audio source into playable Snare or Clap sounds. The first delivery is the standalone app; VST3, CLAP, and AU formats are planned for later.

**Repository status: standalone implementation is in verification.** The app now decodes and analyzes WAV/AIFF sources, generates Snare and Clap renders, supports preview, variation, undo/reset, factory and user presets, session recovery, and WAV export. Windows and macOS builds run through GitHub Actions; platform smoke tests and listening review remain release checks.

## Included workflow

- Load one mono or stereo WAV/AIFF file (up to 10 minutes), analyze it, and inspect its waveform and strongest transient.
- Generate deterministic Snare or Clap renders with six primary macros and a scrollable advanced panel.
- Preview the active render, randomize or mutate parameters, undo the last variation, and reset to defaults.
- Save and load JSON presets, use factory recipes, and recover the previous render and settings at startup.
- Export the active render to WAV as PCM 16-bit, PCM 24-bit, or float 32-bit, with selectable sample rate, channel layout, normalization, and output trim.
- Open the built-in help and turn contextual tooltips on or off from the title bar.

Plugin formats remain deferred until standalone verification is complete. No release binaries are published yet.

## Start here

- [Standalone-first specification](docs/superpowers/specs/standalone-first.md)
- [Standalone-first implementation plan](docs/superpowers/plans/2026-10-06-snaircreator-standalone-first.md)
- [Approved design specification](docs/superpowers/specs/2026-10-03-snaircreator-design.md)
- [Focused specification index](docs/superpowers/specs/README.md)
- [Plugin-format backlog plan](docs/superpowers/plans/2026-10-04-snaircreator-vst-implementation.md)
- [Requirement coverage tracker](docs/spec-coverage.md)
- [Build handoff](TODO.md)
- [Decisions](DECISIONS.md)
- [License](LICENSE)
- [Copyright and trademark notice](COPYRIGHT-TRADEMARK.md)

## Focused specifications

GUI, source ingestion, render engine, standalone behavior, presets/export, and verification/build/release contracts are listed in the [specification index](docs/superpowers/specs/README.md). Plugin formats follow after the standalone workflow is stable.


## Required shared plug-in standard

This project follows the [Circuit Drift Labs Shared Audio Plugin Standard](docs/standards/CDL_PLUGIN_BASELINE.md). It is required for the plug-in target; standalone-only requirements apply only when a standalone target is included. The product-specific specification supplements the shared standard and records the applicable profiles, compliance status, and any exceptions.
