# SnairCreator

SnairCreator is a JUCE/C++ desktop application that will turn a user-provided audio source into playable Snare or Clap sounds. The first delivery is the standalone app; VST3, CLAP, and AU formats are planned for later.

**Repository status: early standalone implementation.** The native app shell and GUI are in place. Audio transformation, playback, and WAV export are still pending; no built binaries are published.

## Start here

- [Standalone-first specification](docs/superpowers/specs/standalone-first.md)\n- [Standalone-first implementation plan](docs/superpowers/plans/2026-10-06-snaircreator-standalone-first.md)\n- [Approved design specification](docs/superpowers/specs/2026-10-03-snaircreator-design.md)
- [Focused specification index](docs/superpowers/specs/README.md)
- [Implementation plan](docs/superpowers/plans/2026-10-04-snaircreator-vst-implementation.md)
- [Requirement coverage tracker](docs/spec-coverage.md)
- [Build handoff](TODO.md)
- [Decisions](DECISIONS.md)
- [License](LICENSE)
- [Copyright and trademark notice](COPYRIGHT-TRADEMARK.md)

## Focused specifications

GUI, source ingestion, render engine, standalone behavior, presets/export, and verification/build/release contracts are listed in the [specification index](docs/superpowers/specs/README.md). Plugin formats follow after the standalone workflow is stable.
