# SnairCreator Design Specification

## 1. Product definition

SnairCreator is a cross-platform sound-design instrument that turns arbitrary source material into a playable, exportable snare or clap. The source may be a drum hit, vocal, field recording, music excerpt, noise, household sound, or any other decodable audio file. The transformation must preserve enough fingerprint from the source to make each result recognizably unique while forcing the output into a useful percussion envelope.

The product ships in four forms:

1. Browser app, written in JavaScript and deployable through GitHub Pages.
2. Python CLI/library for offline processing, automation, and batch use.
3. Native JUCE plugin/standalone application for Windows, macOS, and Linux where supported.
4. Plugin formats: VST3 on Windows/macOS/Linux, CLAP on Windows/macOS/Linux, and Audio Unit (AU) on macOS. A macOS Standalone app is also built.

All variants share the same conceptual processing stages and parameter names so a preset can be understood across implementations.

## 2. Core user flow

1. Load or drag an audio file.
2. SnairCreator decodes the file and analyzes transient density, RMS/peak level, spectral centroid, zero-crossing rate, and an approximate dominant/resonant frequency.
3. The engine creates a deterministic source fingerprint from audio-derived statistics plus an optional seed.
4. Choose Snare, Clap, or Hybrid mode.
5. Adjust macro controls or press Randomize/Mutate.
6. Preview the generated hit immediately.
7. Export a WAV file or trigger the generated sound from MIDI in the plugin.
8. Save/load presets. The native plugin also persists state in the host session.

## 3. DSP architecture

The engine is intentionally procedural rather than a simple one-shot sampler. The source is reduced to reusable sound components:

- **Attack material:** a short window around the strongest onset, high-passed and shaped into the initial crack.
- **Body material:** a resonant layer tuned around a source-derived or user-selected body frequency.
- **Noise material:** decorrelated/high-passed source content used for snare-wire or clap texture.
- **Clap taps:** 2-6 short, jittered bursts derived from the source noise layer.
- **Tail:** filtered/noisy source material shaped by an exponential decay.

Processing order:

`decode -> mono/stereo normalization -> source analysis -> onset selection -> layer extraction -> layer synthesis -> saturation -> tone filter -> trim/normalize -> output`

### Snare mode

A short source transient is mixed with a resonant body and filtered noise tail. Body frequency defaults to a source-derived value clamped to 110-320 Hz. Noise is biased toward 1.5-10 kHz. The result is shaped to a one-shot percussion envelope.

### Clap mode

The engine creates 2-6 micro-bursts from the source, separated by 8-35 ms with deterministic jitter, followed by a wider noisy tail. Stereo width may decorrelate left/right tap timing and polarity.

### Hybrid mode

Snare body/transient plus clap multi-tap texture, with a macro controlling the balance.

## 4. Parameters

Stable parameter IDs/names across implementations:

- `mode`: snare | clap | hybrid
- `seed`: integer, 0-2147483647
- `character`: 0-1; source fingerprint strength versus generic percussion shaping
- `body`: 0-1; resonant body amount
- `body_freq_hz`: 70-450
- `crack`: 0-1; transient/high-frequency attack level
- `noise`: 0-1; wire/noise layer level
- `tail_ms`: 20-1200
- `clap_count`: integer 2-6
- `clap_spread_ms`: 8-35
- `width`: 0-1
- `drive_db`: 0-18
- `tone`: -1 to +1; darker to brighter spectral tilt/filter macro
- `pitch_st`: -24 to +24 semitones for extracted material
- `trim_db`: -24 to +12
- `normalize`: boolean
- `output_ms`: 40-2000

Macros:

- `Punch`: maps body, crack, and transient envelope.
- `Snap`: maps crack, noise brightness, and clap spread.
- `Dirt`: maps drive and source-character weighting.
- `Size`: maps body decay and tail length.

## 5. Deterministic mutation

A generated hit must be reproducible from the same decoded samples, parameter set, and seed. Randomness uses a small deterministic PRNG. `Randomize` changes the seed and several musical parameters within safe ranges. `Mutate` retains the current identity and changes only a bounded subset by +/- 10-20%.

## 6. Input and output

### Browser

Use `AudioContext.decodeAudioData()` for formats supported by the current browser. The UI must say "Load / scan locally" and must never imply that files are uploaded. Processing is local-only. Export is PCM WAV generated in-browser.

### Python

Primary supported input: WAV through the standard library so the base package has no mandatory runtime dependency. Optional `soundfile` support extends decoding to formats available through libsndfile. Output is 16-bit or 24-bit PCM WAV. CLI accepts single files and directories.

### Native

JUCE `AudioFormatManager` handles common formats available in JUCE. The plugin keeps the loaded source in memory and never reads the filesystem from the realtime audio callback. Rendering of a new one-shot happens off the realtime path, then an immutable rendered buffer is swapped into the playback engine.

## 7. Native plugin behavior

The plugin is an instrument with MIDI input. MIDI note-on triggers the rendered one-shot; velocity scales amplitude. Note 36 and 38 are conventional defaults but every note may trigger. Multiple voices may overlap up to 16 voices. The plugin has stereo output and no audio input requirement.

File loading and re-rendering occur on the message/background side. `processBlock()` performs only bounded buffer playback/mixing and parameter reads safe for the audio thread.

The plugin exposes host-automatable synthesis controls. File path/state and the last rendered source fingerprint are serialized in the JUCE state tree. If a session cannot recover the original file, the plugin reports that the source must be reloaded instead of silently substituting another file.

## 8. GUI

Visual direction: Circuit Drift Labs family, but with a focused percussion-lab identity. Dark graphite base, cyan/blue accents, restrained magenta transient highlights, high contrast typography, no faux analog hardware.

Layout:

- Header: SnairCreator, format/version, About.
- Source panel: drag/drop target, file name, duration, mini waveform, analysis readout.
- Mode strip: Snare / Clap / Hybrid.
- Four large macro controls: Punch, Snap, Dirt, Size.
- Detailed controls: body, frequency, crack, noise, tail, clap count/spread, width, drive, tone, pitch, trim.
- Action row: Preview, Randomize, Mutate, Undo Mutation, Reset.
- Export/preset row: Save WAV, Save Preset, Load Preset.
- Native plugin additionally displays MIDI activity.

Browser UI must be responsive down to 360 px width; plugin editor default size 980 x 680 and resizable with a 720 x 500 minimum.

## 9. Browser architecture

- `web/index.html`: semantic shell.
- `web/styles.css`: visual system and responsive layout.
- `web/engine.js`: pure DSP helpers and deterministic generator.
- `web/wav.js`: WAV encoder.
- `web/app.js`: DOM, decode, preview, presets, drag/drop, export.

`engine.js` must avoid DOM dependencies so Node-based unit tests can exercise it.

## 10. Python architecture

- `python/snaircreator/__init__.py`: public API/version.
- `python/snaircreator/engine.py`: DSP and deterministic PRNG.
- `python/snaircreator/audioio.py`: WAV decoding/encoding and optional soundfile path.
- `python/snaircreator/cli.py`: argparse command.
- `python/tests/`: analysis, determinism, rendering, CLI tests.

Public API:

```python
analyze(samples, sample_rate) -> Analysis
render(samples, sample_rate, params) -> list[float]
render_file(input_path, output_path, params) -> RenderReport
```

## 11. Native architecture

- `native/CMakeLists.txt`: JUCE plus clap-juce-extensions via FetchContent.
- `native/Source/SnairEngine.{h,cpp}`: analysis and offline one-shot render.
- `native/Source/PluginProcessor.{h,cpp}`: APVTS, state, MIDI voice playback, source loading.
- `native/Source/PluginEditor.{h,cpp}`: complete JUCE GUI and file drag/drop.
- `native/tests/`: engine tests where practical.

JUCE targets: VST3, AU, Standalone. CLAP target is added through `clap_juce_extensions_plugin`.

## 12. Preset interchange

Browser and Python presets use JSON with:

```json
{
  "schema": 1,
  "product": "SnairCreator",
  "parameters": {},
  "seed": 1234
}
```

Native plugin may use JUCE state serialization internally but parameter IDs remain identical. Future cross-import may wrap the same JSON schema.

## 13. Error handling

- Reject empty/zero-frame audio.
- Reject non-finite decoded sample data.
- Limit source analysis to a bounded duration for very large files while preserving the strongest transient candidate.
- Clamp every public parameter at the engine boundary.
- Never allow NaN/Inf into generated output.
- Browser displays decode/export errors in a visible status region.
- Python returns nonzero exit codes and concise stderr messages.
- Plugin reports load/render errors in the GUI without blocking the audio thread.

## 14. Testing and acceptance criteria

Required automated coverage:

- Deterministic render for identical seed/input/parameters.
- Different seeds produce measurably different results.
- Silent input still yields a finite, bounded percussion result rather than NaN.
- Very short input does not index outside buffers.
- Stereo inputs can be reduced/used safely.
- Parameter clamping prevents invalid envelopes and tap counts.
- Output is peak-bounded to <= 1.0 when normalize is enabled.
- Browser engine unit tests run under Node without a browser.
- Python tests run under pytest/unittest without external audio fixtures by generating synthetic samples.
- Native project configures with CMake on supported runners; plugin build workflow covers Windows and macOS.

Manual acceptance:

- Browser: load, preview, switch all three modes, randomize, mutate, undo, export WAV, save/load preset.
- Python: render one file and batch directory.
- VST3/CLAP/AU/Standalone: load source, render, trigger by MIDI, automate controls, reload host state.

## 15. CI and releases

GitHub Actions:

- `web-python-tests.yml`: Node tests plus Python tests on push/PR.
- `native-build.yml`: Windows and macOS native configure/build. Upload VST3/CLAP artifacts; macOS additionally uploads AU and Standalone.
- `pages.yml`: deploy `web/` to GitHub Pages using the official Pages actions.

Native release artifacts are unsigned beta builds. macOS users may need to authorize unsigned builds locally until signing/notarization is configured.

## 16. Out of scope for beta

- Machine-learning source separation.
- Cloud processing or accounts.
- AAX.
- Mobile native apps.
- Guaranteed decoding of literally every file format; "any file" means any decodable audio file on the target platform.
- Built-in sample marketplace/library.

## 17. Definition of done

The repository contains the complete spec, implementation plan, browser app, Python package/CLI, JUCE source, CMake configuration, tests, CI workflows, build/install/user documentation, license, and audit report. The audit compares implementation to this document and records any remaining platform-dependent limitations explicitly.
