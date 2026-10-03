# SnairCreator Beta Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a browser app, Python CLI/library, and native JUCE VST3/CLAP/AU/Standalone instrument that transforms arbitrary decodable audio into unique snare/clap hits.

**Architecture:** Keep one stable parameter model and one deterministic procedural recipe across all targets. Each platform has its own implementation language, but the stages and IDs remain aligned: analyze source, derive deterministic fingerprint, extract layers, synthesize snare/clap/hybrid, shape/master, preview/export/trigger.

**Tech Stack:** JavaScript/Web Audio API, Node tests, Python 3 standard library with optional soundfile, JUCE 8/C++17/CMake, free-audio/clap-juce-extensions, GitHub Actions/Pages.

**Spec:** `docs/superpowers/specs/2026-10-03-snaircreator-design.md`

## Global Constraints

- Browser processing is local-only and deployable as static GitHub Pages content.
- Stable parameter IDs match the specification.
- Same input + seed + parameters is deterministic within each implementation.
- Native realtime callback never performs file IO or synthesis rendering.
- Native formats: VST3, CLAP, AU on macOS, Standalone.
- Browser and Python export WAV.
- Beta builds are unsigned unless signing is separately configured.

## Review Focus

- Silent/near-silent source must render finite bounded output.
- Inputs shorter than one clap spread window must not index out of range.
- Extreme/invalid parameter values must be clamped.
- Repeated renders with same seed must match; changed seed must vary.
- Host realtime path must remain allocation/file-IO free during ordinary playback.

---

### Task 1: Browser DSP engine and tests

**Files:** `web/engine.js`, `web/wav.js`, `web/tests/engine.test.mjs`, `package.json`

**Interfaces:**
- `analyze(samples, sampleRate)` -> analysis object.
- `render(samples, sampleRate, params)` -> `Float32Array`.
- `encodeWav(samples, sampleRate, bitDepth)` -> `ArrayBuffer`.

- [ ] Write deterministic, seed-variation, silence, short-input, clamp, and normalized-peak tests.
- [ ] Run `npm test` and confirm tests fail before engine exists.
- [ ] Implement deterministic PRNG, analysis, renderer, and WAV encoder.
- [ ] Run `npm test` and confirm all tests pass.

### Task 2: Browser application and Pages

**Files:** `web/index.html`, `web/styles.css`, `web/app.js`, `.github/workflows/pages.yml`

**Interfaces:** consumes Task 1 engine/encoder.

- [ ] Implement local drag/drop/file picker and visible local-processing copy.
- [ ] Implement mode/macros/details, preview, randomize, mutate/undo, reset.
- [ ] Implement WAV export and JSON preset save/load.
- [ ] Implement responsive Circuit Drift Labs-derived GUI.
- [ ] Add Pages deployment workflow for `web/`.

### Task 3: Python engine, IO, CLI, tests

**Files:** `python/pyproject.toml`, `python/snaircreator/{__init__,engine,audioio,cli}.py`, `python/tests/test_engine.py`

**Interfaces:**
- `analyze(samples, sample_rate) -> Analysis`
- `render(samples, sample_rate, params) -> list[float]`
- `render_file(input_path, output_path, params) -> RenderReport`

- [ ] Write synthetic-fixture tests covering Review Focus items.
- [ ] Run tests and confirm initial failures.
- [ ] Implement engine and standard-library WAV IO plus optional soundfile decoder.
- [ ] Implement single-file and recursive directory CLI rendering.
- [ ] Run `python -m unittest discover -s python/tests -v`.

### Task 4: Native DSP and plugin

**Files:** `native/CMakeLists.txt`, `native/Source/SnairEngine.{h,cpp}`, `native/Source/PluginProcessor.{h,cpp}`, `native/Source/PluginEditor.{h,cpp}`

**Interfaces:**
- `SnairEngine::analyse(...)`
- `SnairEngine::render(...)`
- processor method `loadSourceFile(const juce::File&)`
- APVTS parameter IDs equal spec.

- [ ] Add pure engine tests where CMake runner supports them.
- [ ] Implement analysis/render engine with deterministic seed.
- [ ] Implement APVTS/state and 16-voice MIDI one-shot playback.
- [ ] Ensure source load/render is outside `processBlock`.
- [ ] Implement resizable full GUI, drag/drop, source readout, preview/randomize/mutate/reset.
- [ ] Configure JUCE VST3/AU/Standalone and CLAP extension target.

### Task 5: CI, docs, audit

**Files:** `.github/workflows/web-python-tests.yml`, `.github/workflows/native-build.yml`, `README.md`, `docs/BUILDING.md`, `docs/AUDIT.md`, `LICENSE`

- [ ] Add JS/Python test CI.
- [ ] Add Windows/macOS native build matrix and artifact uploads.
- [ ] Document install/build/use for all targets.
- [ ] Audit every spec section against repository files and workflows.
- [ ] Fix gaps found, then record only genuine platform/signing limitations in `docs/AUDIT.md`.
