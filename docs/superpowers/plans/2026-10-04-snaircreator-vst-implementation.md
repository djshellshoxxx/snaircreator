# SnairCreator VST Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build SnairCreator as a native JUCE/C++ instrument that transforms arbitrary decodable audio into playable Snare or Clap hits, supports MIDI triggering in a DAW, and exports the exact current result to WAV.

**Architecture:** Use a realtime-safe split between offline source analysis/rendering and a lightweight playback engine. Source loading, analysis, mutation, and one-shot rendering happen off the audio callback; the audio thread only reads immutable rendered hit data, MIDI events, and atomically readable parameters. The same DSP engine serves VST3, CLAP, AU on macOS, and Standalone targets.

**Tech Stack:** C++20, JUCE 8.x, CMake 3.24+, clap-juce-extensions, Catch2 or JUCE UnitTest for native tests, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-10-03-snaircreator-design.md`

## Global Constraints

- Primary user modes are exactly `Snare` and `Clap`; no top-level Hybrid mode.
- VST3 and CLAP are primary plugin targets; AU is included on macOS; Standalone is also built.
- WAV and AIFF source decoding are mandatory.
- Source files are fully decoded into plugin-owned memory before rendering; `processBlock()` must never perform filesystem I/O.
- Analysis and one-shot rendering must never run on the realtime audio thread.
- MIDI note-on triggers the current rendered hit; velocity scales amplitude; overlapping hits are supported.
- The Advanced panel must not be required for normal use.
- Export must render the exact currently generated sound.
- WAV export must support 16-bit PCM, 24-bit PCM, and 32-bit float.
- Identical source audio + identical parameters + identical seed must produce identical rendered output.
- Public parameters must be clamped and generated audio must never contain NaN or Inf.
- Project/session restore must preserve parameter state and the currently generated hit; missing source files must not silently substitute other material.

## Review Focus

- Very short or silent source files: generation remains finite, bounded, and does not read out of range.
- Long source files with useful transients late in the file: analysis still finds viable transient candidates instead of using only the beginning.
- Host sample-rate changes after a source is loaded: rendered playback remains pitch/time correct and regeneration behavior is deterministic.
- Source file becomes unavailable after project save: reopening the project restores the generated hit and reports source-unavailable state without breaking playback.
- Rapid MIDI plus repeated GUI mutations: audio callback stays glitch-free while rendered buffers are swapped safely.

---

### Task 1: Native project skeleton and build targets

**Files:**
- Create: `CMakeLists.txt`
- Create: `native/CMakeLists.txt`
- Create: `native/Source/PluginProcessor.h`
- Create: `native/Source/PluginProcessor.cpp`
- Create: `native/Source/PluginEditor.h`
- Create: `native/Source/PluginEditor.cpp`
- Create: `native/tests/CMakeLists.txt`
- Create: `.gitignore`

**Interfaces:**
- Produces: JUCE plugin target `SnairCreator` with VST3, Standalone, CLAP, and conditional AU formats.
- Produces: `SnairCreatorAudioProcessor` and `SnairCreatorAudioProcessorEditor` classes.

- [ ] **Step 1: Write the failing configure/build smoke test**

Create a CI/local smoke check that configures the project with `SNAIRCREATOR_BUILD_TESTS=ON` and expects target generation for the native library and tests.

- [ ] **Step 2: Run configure and verify failure**

Run: `cmake -S . -B build -DSNAIRCREATOR_BUILD_TESTS=ON`
Expected: FAIL because the native project has not yet been defined.

- [ ] **Step 3: Implement the minimal JUCE/CMake project**

Use C++20. Add JUCE through `FetchContent` or a pinned submodule-compatible path. Add clap-juce-extensions for CLAP. Keep plugin processor/editor implementations minimal but instantiable.

- [ ] **Step 4: Configure and build**

Run: `cmake -S . -B build -DSNAIRCREATOR_BUILD_TESTS=ON && cmake --build build --config Release`
Expected: PASS and produce native targets without DSP functionality.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt native .gitignore
git commit -m "build: scaffold native SnairCreator plugin"
```

### Task 2: Core model, parameters, and deterministic random engine

**Files:**
- Create: `native/Source/Model/SnairParameters.h`
- Create: `native/Source/Model/SnairParameters.cpp`
- Create: `native/Source/DSP/DeterministicRng.h`
- Create: `native/tests/ParameterTests.cpp`
- Create: `native/tests/RngTests.cpp`

**Interfaces:**
- Produces: `enum class PercussionMode { Snare, Clap };`
- Produces: `struct SnairParameters` with stable parameter IDs and clamping helpers.
- Produces: `class DeterministicRng { explicit DeterministicRng(uint32_t seed); uint32_t nextU32(); float nextFloat(); };`

- [ ] **Step 1: Write failing parameter tests**

Test that mode only accepts Snare/Clap; body frequency, clap count/spread, pitch, tail, width, drive, trim, character, and macro values clamp to spec ranges; invalid floating values are sanitized.

- [ ] **Step 2: Write failing deterministic RNG tests**

Assert equal seeds return identical sequences and different seeds return different sequences over the first 16 values.

- [ ] **Step 3: Run tests and verify failure**

Run: `ctest --test-dir build -R "Parameter|Rng" --output-on-failure`
Expected: FAIL because model/RNG types do not exist.

- [ ] **Step 4: Implement model and RNG**

Use a small fixed algorithm such as PCG32 or xorshift32 with explicitly stable behavior across platforms.

- [ ] **Step 5: Run tests and commit**

Run: `ctest --test-dir build -R "Parameter|Rng" --output-on-failure`
Expected: PASS.

```bash
git add native/Source/Model native/Source/DSP/DeterministicRng.h native/tests
git commit -m "feat: add stable parameters and deterministic RNG"
```

### Task 3: Source loading and source analysis

**Files:**
- Create: `native/Source/Audio/SourceAudio.h`
- Create: `native/Source/Audio/SourceLoader.h`
- Create: `native/Source/Audio/SourceLoader.cpp`
- Create: `native/Source/DSP/SourceAnalysis.h`
- Create: `native/Source/DSP/SourceAnalyzer.h`
- Create: `native/Source/DSP/SourceAnalyzer.cpp`
- Create: `native/tests/SourceAnalyzerTests.cpp`

**Interfaces:**
- Produces: `struct SourceAudio { juce::AudioBuffer<float> samples; double sampleRate; juce::String sourceName; };`
- Produces: `std::optional<SourceAudio> SourceLoader::load(const juce::File&, juce::String& error);`
- Produces: `SourceAnalysis SourceAnalyzer::analyze(const SourceAudio& source);`

- [ ] **Step 1: Write failing synthetic-source tests**

Cover impulse, sine+noise, stereo, silent, 1-sample, non-finite sanitized input, and a long buffer whose strongest transient is near the end. Assert finite metrics, valid transient bounds, and late-transient discovery.

- [ ] **Step 2: Run tests and verify failure**

Run: `ctest --test-dir build -R SourceAnalyzer --output-on-failure`
Expected: FAIL.

- [ ] **Step 3: Implement source representation and loader**

Use JUCE `AudioFormatManager`; require WAV/AIFF reader registration. Copy decoded samples into owned memory and sanitize non-finite values before analysis.

- [ ] **Step 4: Implement analyzer**

Compute duration, peak, RMS, crest factor, transient candidates, transient density, centroid, rolloff, flatness, zero-crossing rate, band-energy ratios, body resonance estimate, noisy/tonal estimate, attack/tail regions, and deterministic source fingerprint.

- [ ] **Step 5: Run tests and commit**

Run: `ctest --test-dir build -R SourceAnalyzer --output-on-failure`
Expected: PASS.

```bash
git add native/Source/Audio native/Source/DSP/SourceAnalysis.h native/Source/DSP/SourceAnalyzer.* native/tests/SourceAnalyzerTests.cpp
git commit -m "feat: add source loading and analysis"
```

### Task 4: Snare and Clap offline rendering engine

**Files:**
- Create: `native/Source/DSP/RenderedHit.h`
- Create: `native/Source/DSP/SnairEngine.h`
- Create: `native/Source/DSP/SnairEngine.cpp`
- Create: `native/tests/SnairEngineTests.cpp`

**Interfaces:**
- Consumes: `SourceAudio`, `SourceAnalysis`, `SnairParameters`, `DeterministicRng`.
- Produces: `RenderedHit SnairEngine::render(const SourceAudio&, const SourceAnalysis&, const SnairParameters&, double outputSampleRate);`
- Produces: `struct RenderedHit { juce::AudioBuffer<float> samples; double sampleRate; uint64_t fingerprint; };`

- [ ] **Step 1: Write failing Snare tests**

Assert output is finite/non-empty, peak-bounded when normalize is enabled, body energy exists in the configured range, changing Source Character audibly changes output, and identical input/params/seed produce sample-identical output.

- [ ] **Step 2: Write failing Clap tests**

Assert clap mode creates multiple distinct early energy bursts for `clap_count >= 2`, spread reacts to `clap_spread_ms`, width affects L/R difference, and silent/very-short source remains finite.

- [ ] **Step 3: Run tests and verify failure**

Run: `ctest --test-dir build -R SnairEngine --output-on-failure`
Expected: FAIL.

- [ ] **Step 4: Implement Snare rendering**

Construct source-derived attack, resonant body, texture/noise, tail, tone, drive, pitch, envelope, width, trim, and normalization stages. Keep rendering fully offline.

- [ ] **Step 5: Implement Clap rendering**

Construct 2-6 deterministic micro-bursts from source-derived transient/noise material, apply configured spread/jitter, stereo width, tail, tone, drive, trim, and normalization.

- [ ] **Step 6: Run tests and commit**

Run: `ctest --test-dir build -R SnairEngine --output-on-failure`
Expected: PASS.

```bash
git add native/Source/DSP/RenderedHit.h native/Source/DSP/SnairEngine.* native/tests/SnairEngineTests.cpp
git commit -m "feat: render source-derived snare and clap hits"
```

### Task 5: Realtime-safe MIDI playback and rendered-buffer swapping

**Files:**
- Create: `native/Source/Playback/HitVoice.h`
- Create: `native/Source/Playback/HitPlaybackEngine.h`
- Create: `native/Source/Playback/HitPlaybackEngine.cpp`
- Modify: `native/Source/PluginProcessor.h`
- Modify: `native/Source/PluginProcessor.cpp`
- Create: `native/tests/PlaybackEngineTests.cpp`

**Interfaces:**
- Produces: `void HitPlaybackEngine::setRenderedHit(std::shared_ptr<const RenderedHit>);`
- Produces: `void HitPlaybackEngine::process(juce::AudioBuffer<float>&, const juce::MidiBuffer&);`
- Supports at least 16 overlapping voices.

- [ ] **Step 1: Write failing playback tests**

Assert MIDI note-on triggers audio, velocity scales level, 16 overlapping voices mix without invalid output, note-off does not truncate one-shots, and swapping rendered hit data between blocks does not invalidate active voices.

- [ ] **Step 2: Run tests and verify failure**

Run: `ctest --test-dir build -R PlaybackEngine --output-on-failure`
Expected: FAIL.

- [ ] **Step 3: Implement playback engine**

Use immutable shared rendered-hit ownership or an equivalent lock-free-safe handoff. Do not allocate, lock, load files, analyze, or render hits inside `processBlock()`.

- [ ] **Step 4: Wire processor MIDI/audio path**

Processor must behave as an instrument with stereo output and MIDI input.

- [ ] **Step 5: Run tests and commit**

Run: `ctest --test-dir build -R PlaybackEngine --output-on-failure`
Expected: PASS.

```bash
git add native/Source/Playback native/Source/PluginProcessor.* native/tests/PlaybackEngineTests.cpp
git commit -m "feat: add realtime-safe MIDI hit playback"
```

### Task 6: Parameter state, rendering jobs, Randomize/Mutate, and session restore

**Files:**
- Create: `native/Source/State/ParameterLayout.h`
- Create: `native/Source/State/ParameterLayout.cpp`
- Create: `native/Source/State/RenderController.h`
- Create: `native/Source/State/RenderController.cpp`
- Modify: `native/Source/PluginProcessor.h`
- Modify: `native/Source/PluginProcessor.cpp`
- Create: `native/tests/StateTests.cpp`

**Interfaces:**
- Produces: JUCE `AudioProcessorValueTreeState` with stable IDs from `SnairParameters`.
- Produces: `void RenderController::requestRender(RenderRequest);`
- Produces: `void RenderController::randomize();`
- Produces: `void RenderController::mutate();`
- Produces: `void RenderController::undoMutation();`

- [ ] **Step 1: Write failing state tests**

Assert parameter round-trip, seed persistence, rendered-hit serialization/restoration, deterministic Randomize by resulting seed, bounded Mutate deltas, Undo Mutation restoration, and recovery when original source path is missing.

- [ ] **Step 2: Run tests and verify failure**

Run: `ctest --test-dir build -R State --output-on-failure`
Expected: FAIL.

- [ ] **Step 3: Implement APVTS and background render controller**

Any parameter change that requires regeneration schedules work off the realtime thread and atomically publishes the completed `RenderedHit`.

- [ ] **Step 4: Implement session serialization**

Persist parameters, seed, source metadata, source fingerprint, and enough rendered-hit data to restore playback even if the source file is unavailable.

- [ ] **Step 5: Run tests and commit**

Run: `ctest --test-dir build -R State --output-on-failure`
Expected: PASS.

```bash
git add native/Source/State native/Source/PluginProcessor.* native/tests/StateTests.cpp
git commit -m "feat: add render control and persistent plugin state"
```

### Task 7: WAV export

**Files:**
- Create: `native/Source/Export/WavExporter.h`
- Create: `native/Source/Export/WavExporter.cpp`
- Create: `native/tests/WavExporterTests.cpp`

**Interfaces:**
- Produces: `enum class WavBitDepth { PCM16, PCM24, Float32 };`
- Produces: `bool WavExporter::write(const RenderedHit&, const juce::File&, WavBitDepth, juce::String& error);`

- [ ] **Step 1: Write failing export tests**

Export the same known rendered hit at 16-bit, 24-bit, and 32-bit float, reopen each with JUCE, and assert channel count, sample rate, approximate duration, and expected sample-format depth.

- [ ] **Step 2: Run tests and verify failure**

Run: `ctest --test-dir build -R WavExporter --output-on-failure`
Expected: FAIL.

- [ ] **Step 3: Implement exporter**

Export exactly the current immutable rendered hit; do not regenerate during export. Use atomic/temporary-file replacement where practical to avoid partial output.

- [ ] **Step 4: Run tests and commit**

Run: `ctest --test-dir build -R WavExporter --output-on-failure`
Expected: PASS.

```bash
git add native/Source/Export native/tests/WavExporterTests.cpp
git commit -m "feat: export generated hits to WAV"
```

### Task 8: Full plugin GUI and interaction wiring

**Files:**
- Create: `native/Source/UI/Theme.h`
- Create: `native/Source/UI/WaveformView.h`
- Create: `native/Source/UI/WaveformView.cpp`
- Create: `native/Source/UI/SourcePanel.h`
- Create: `native/Source/UI/SourcePanel.cpp`
- Create: `native/Source/UI/MacroPanel.h`
- Create: `native/Source/UI/MacroPanel.cpp`
- Create: `native/Source/UI/AdvancedPanel.h`
- Create: `native/Source/UI/AdvancedPanel.cpp`
- Modify: `native/Source/PluginEditor.h`
- Modify: `native/Source/PluginEditor.cpp`

**Interfaces:**
- Consumes: APVTS, `RenderController`, `WavExporter`, current `SourceAudio`/analysis display state.
- Produces: drag/drop and file chooser loading; Snare/Clap switch; waveform; six macro controls; Advanced controls; Preview, Randomize, Mutate, Undo, Reset, Export WAV; status/error display; MIDI activity indication.

- [ ] **Step 1: Add GUI smoke tests or component-level state tests**

Test at minimum: editor instantiates headlessly, mode selector updates APVTS, Advanced panel toggles without changing parameters, source-drop dispatch validates file type, and export action is disabled before a rendered hit exists.

- [ ] **Step 2: Run tests and verify failure**

Run: `ctest --test-dir build -R UI --output-on-failure`
Expected: FAIL.

- [ ] **Step 3: Implement the editor layout**

Default editor size: `980 x 680`; minimum resizable size: `720 x 500`. Keep primary workflow usable with Advanced collapsed.

- [ ] **Step 4: Wire all actions**

Load/drag source, Snare/Clap selection, Preview, macros, Advanced parameters, Randomize, Mutate, Undo Mutation, Reset, WAV export, status/errors, and MIDI indicator must all invoke the existing controller/state interfaces.

- [ ] **Step 5: Run tests and manual standalone check**

Run: `ctest --test-dir build -R UI --output-on-failure`
Expected: PASS.

Manual: launch Standalone, load WAV, switch Snare/Clap, play Preview, randomize, mutate/undo, and export WAV.

- [ ] **Step 6: Commit**

```bash
git add native/Source/UI native/Source/PluginEditor.*
git commit -m "feat: add complete SnairCreator plugin interface"
```

### Task 9: CI, packaging, documentation, and acceptance audit

**Files:**
- Create: `.github/workflows/native-build.yml`
- Create: `README.md`
- Create: `docs/BUILDING.md`
- Create: `docs/USER_GUIDE.md`
- Create: `docs/AUDIT.md`

**Interfaces:**
- Produces: reproducible Windows/macOS build workflows and documented Linux build path.
- Produces: VST3/CLAP artifacts on supported runners; AU and Standalone on macOS.

- [ ] **Step 1: Add CI workflow**

Build Release configuration and run tests on Windows and macOS. Upload built plugin artifacts. Keep dependency revisions pinned.

- [ ] **Step 2: Write user/build documentation**

Document installation paths, supported source files, Snare/Clap workflow, MIDI use, Source Character, Randomize/Mutate, WAV export, unsigned-build limitations, and source-missing session behavior.

- [ ] **Step 3: Run complete verification locally where supported**

Run:

```bash
cmake -S . -B build -DSNAIRCREATOR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests PASS.

- [ ] **Step 4: Perform spec acceptance audit**

Compare every section of `docs/superpowers/specs/2026-10-03-snaircreator-design.md` against implemented behavior. Record platform-dependent limitations and any deferred non-beta items in `docs/AUDIT.md`; do not silently omit missing requirements.

- [ ] **Step 5: Manual DAW acceptance**

Verify at least one VST3 host and one CLAP-capable host: load source, generate Snare/Clap, MIDI-trigger with velocity, overlap hits, automate controls, save/reopen project, regenerate, and export current sound to WAV.

- [ ] **Step 6: Commit**

```bash
git add .github README.md docs
git commit -m "ci: add builds docs and acceptance audit"
```

## Final Verification Gate

Before claiming beta completion:

- [ ] Configure/build succeeds from a clean checkout.
- [ ] All native automated tests pass.
- [ ] VST3 loads in a real host.
- [ ] CLAP loads in a real host.
- [ ] AU loads on macOS when built there.
- [ ] Standalone launches and performs the full generation/export workflow.
- [ ] Snare and Clap both transform at least five materially different source categories.
- [ ] MIDI velocity and overlapping voices behave correctly.
- [ ] Saving/reopening a project restores playback with and without the original source file present.
- [ ] WAV exports reopen correctly at 16-bit PCM, 24-bit PCM, and 32-bit float.
- [ ] `docs/AUDIT.md` contains no unacknowledged spec gaps.
