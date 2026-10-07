# SnairCreator

SnairCreator is a JUCE/C++ percussion instrument by Circuit Drift Labs. It turns any WAV or AIFF sound into a playable **snare** or **clap** while keeping some of the source's character.

**Status: v0.0.1 beta.** Standalone, VST3 and CLAP on Windows, macOS (plus AU) and Linux. Download from the [Releases page](https://github.com/djshellshoxxx/snaircreator/releases).

## Features

- Load or drag in one WAV/AIFF source. It is analyzed off the audio thread (transients, spectrum, body resonance, noisiness).
- **Snare** (attack crack + modal shell body + source-grain wires) and **Clap** (2–6 deterministic micro-bursts + diffused texture tail) engines.
- Source Character, six macros (Punch, Snap, Body, Texture, Dirt, Size) and an Advanced panel with every parameter from design spec §12.
- Preview, MIDI playback (any note, velocity, 16 one-shot voices, sample-accurate), Randomize / Mutate / 16-level Undo / Reset.
- Exact-render WAV export (16/24-bit PCM, 32-bit float, mono/stereo, 44.1–96 kHz). Exports go to a temp file, get validated, then move into place.
- Factory and user presets (`.snairpreset`). Host sessions and the standalone app embed the rendered hit, so the sound comes back even if the source file moved.

### New in v0.0.1

| Category | Feature |
|---|---|
| More usable | **Drag-out:** click the waveform to view the generated hit, then drag it straight into your DAW or file browser. Keyboard shortcuts too: Space, R, M, Ctrl/Cmd+Z. |
| More valuable | **Export Kit:** writes the current sound plus 7 coherent mutations as a named WAV sample-kit folder in one click. |
| More fun | **Key Track:** play the hit chromatically across the MIDI keyboard (C3 = original pitch), for pitched snare rolls and clap melodies. |
| Random effect | **Gated Room:** an 80s-style gated reverb burst rendered into the hit (Size lengthens the gate). |

## Documentation

- [Build guide](BUILDING.md) · [Release notes](docs/RELEASE_NOTES.md) · [Spec coverage audit](docs/spec-coverage.md) · [Decisions](DECISIONS.md) · [TODO](TODO.md)
- [Approved design specification](docs/superpowers/specs/2026-10-03-snaircreator-design.md) · [Focused specification index](docs/superpowers/specs/README.md)
- [Circuit Drift Labs Shared Audio Plugin Standard](docs/standards/CDL_PLUGIN_BASELINE.md). This is the required shared baseline; its compliance record is in the [spec coverage audit](docs/spec-coverage.md#cdl-baseline-compliance).
- In-app **HELP** has the full user guide.

## Privacy

SnairCreator makes no network connections and has no telemetry, accounts or licence checks. Audio stays on your machine.

## License

See [LICENSE](LICENSE) and [COPYRIGHT-TRADEMARK.md](COPYRIGHT-TRADEMARK.md). JUCE and clap-juce-extensions are third-party dependencies under their own licences (see BUILDING.md).
