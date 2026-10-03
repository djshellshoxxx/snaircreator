# SnairCreator

SnairCreator turns locally loaded audio into a new snare, clap, or hybrid one-shot. The source can be a drum hit, voice, field recording, loop, household sound, noise recording, or any other audio file the selected runtime can decode. Instead of only trimming a sample, SnairCreator analyzes the source and rebuilds it as percussion using a transient layer, source-derived texture, resonant body, deterministic clap micro-bursts, tail shaping, drive, pitch and tone controls.

This repository contains three implementations of the same design:

- **Browser / JavaScript:** static local-only web app for GitHub Pages.
- **Python:** library plus command-line and directory batch renderer.
- **Native JUCE:** VST3, CLAP, macOS AU, and Standalone builds. Windows and macOS builds are produced in CI; the JUCE/CMake project is also suitable for local Linux VST3/CLAP builds.

Beta version: **0.1.0-beta.1**.

## Browser app

After the main branch is deployed, the Pages workflow publishes `web/` at the repository's GitHub Pages URL, normally:

`https://djshellshoxxx.github.io/snaircreator/`

Choose or drag an audio file. The browser decodes it locally with the Web Audio API. The file is **not uploaded**. Select Snare, Clap or Hybrid, adjust the controls, preview the hit, Randomize or Mutate it, then export a 24-bit WAV or save the parameter preset as JSON.

Browser codec support depends on the browser and operating system. WAV and MP3 are broadly usable; FLAC/AAC/other formats vary by platform.

## Python CLI

Python 3.10+ is required.

```bash
cd python
python -m pip install -e .
```

Base installation supports PCM WAV using the Python standard library. To add formats supported by libsndfile:

```bash
python -m pip install -e '.[formats]'
```

Render one file:

```bash
snaircreator source.wav -o new-snare.wav --mode snare --seed 42 --drive-db 7 --tone 0.25
```

Render a directory:

```bash
snaircreator ./source-folder -o ./generated --mode hybrid --recursive --seed 500
```

Important flags include `--character`, `--body`, `--body-freq`, `--crack`, `--noise`, `--tail-ms`, `--clap-count`, `--clap-spread-ms`, `--width`, `--drive-db`, `--tone`, `--pitch-st`, `--trim-db`, `--output-ms`, `--bit-depth`, and `--no-normalize`.

## Native plugin / app

The native target is a MIDI instrument. Load a source file in the GUI, then play any MIDI note to trigger the generated one-shot. Up to 16 hits can overlap. Velocity controls hit amplitude. The plugin exposes automatable parameters for mode, seed, character, body, body frequency, crack, noise, tail, clap count/spread, width, drive, tone, pitch, trim, normalization and output length.

The native GUI includes a source waveform and analysis readout, Snare/Clap/Hybrid selector, Punch/Snap/Dirt/Size macros, all detailed parameters, Preview, Randomize, Mutate, Undo Mutation, Reset, WAV export, and `.snairpreset` save/load. Host state remembers the source path and reports clearly if the source must be reloaded on another machine.

Build outputs:

- Windows: VST3, CLAP, Standalone
- macOS: VST3, CLAP, AU, Standalone `.app`
- Linux: source project supports VST3/CLAP/Standalone when built locally with the required JUCE platform packages

Generated beta artifacts from GitHub Actions are unsigned. macOS signing/notarization and Windows code signing are intentionally separate release-engineering steps.

See [`docs/BUILDING.md`](docs/BUILDING.md) for build commands and plugin install locations.

## What makes a hit unique

SnairCreator analyzes peak/RMS level, zero-crossing behavior, source onset and a source-derived body-frequency hint. The renderer uses the selected source samples as transient/noise texture and combines them with a deterministic seed. Given the same source, parameters and seed, a target implementation produces the same hit again. Changing the seed or source produces a different percussion identity.

`width` controls how strongly clap taps are micro-shifted and decorrelated. `character` controls how much source fingerprint survives versus synthesized percussion texture.

## Tests

JavaScript:

```bash
npm test
```

Python:

```bash
python -m unittest discover -s python/tests -v
```

Native tests are built through CMake and run with CTest. GitHub Actions runs all three test lanes and builds Windows/macOS plugin artifacts.

## Repository map

- `web/` browser app, DSP and WAV encoder
- `python/` Python package and CLI
- `native/` JUCE plugin/app and native DSP tests
- `docs/superpowers/specs/` design specification
- `docs/superpowers/plans/` implementation plan
- `docs/AUDIT.md` final spec/code audit
- `.github/workflows/` tests, native builds and Pages deployment

## License

MIT. Third-party dependencies such as JUCE, CLAP and clap-juce-extensions retain their own licenses and terms.
