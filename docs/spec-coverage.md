# SnairCreator Specification Coverage

The standalone implementation is feature-complete for its current scope and is in cross-platform verification. Plug-in formats, MIDI host behavior, and host automation remain deferred until the standalone workflow is accepted. Numerical tests do not replace listening or platform launch checks.

| ID | Acceptance | Implementation | Evidence / remaining check | Status |
|---|---|---|---|---|
| BUILD-001 | Clean C++20/CMake/JUCE 9.0.3 standalone build | `CMakeLists.txt`, `.github/workflows/build.yml` | macOS build passed on commit `6b0e1a27`; latest WAV/AIFF extension guard is running through both platform jobs | in progress |
| SRC-001 | Decode WAV/AIFF through chooser or drop; retain last valid source on error | `SourceLoader.cpp`, `MainComponent.cpp` | WAV/AIFF, malformed file, and disguised extension regression tests; latest CI pending | in progress |
| SRC-002 | Handle silence, short, mono/stereo, nonfinite, long, and malformed sources | `SourceLoader.cpp`, `SourceAnalyzer.cpp` | Mono/stereo WAV and AIFF, malformed file, silence, finite sanitation, duration and decoded-memory bounds are implemented; add direct tests for every boundary | in progress |
| SRC-003 | Keep file IO, decode, and analysis off audio callback | `MainComponent.cpp` source worker and audio callback | Decode/analysis run on worker; callback only reads active render and copies samples; manual audio-device smoke test remains | in progress |
| DSP-001 | Versioned analysis/render identity contracts | `SourceAnalysis.h`, `RenderedHit.h` | Source fingerprint, seed, and generation identity exist; explicit schema versions and parameter snapshot identity are absent | pending |
| DSP-002 | Bounded Snare rendering from source and reinforcement | `SnareRenderer.cpp`, `SnairEngine.cpp` | Native tests cover finite output, peak bounds, tone response, rate conversion, and seed changes; listening review remains | in progress |
| DSP-003 | Deterministic Clap pattern with 2–6 bursts | `ClapRenderer.cpp`, `SnairEngine.cpp` | Native test covers bounded deterministic Clap output; direct assertions for count/spread response and cross-platform seed identity remain | in progress |
| DSP-004 | Reproducible render, stale-job discard, and active-render retention | `DeterministicRng.h`, `MainComponent.cpp` | Seeded engine tests pass on macOS; request IDs discard stale worker results; cross-platform exact-sequence test remains | in progress |
| DSP-005 | Sanitize samples, fade edges, and protect output peaks | `SnairEngine.cpp` safety stage | Render tests assert finite samples and peak bounds; edge-fade assertions remain | in progress |
| GUI-001 | Resizable 1000×700 default and 760×520 minimum | `Main.cpp`, `MainComponent.cpp` | Compact layout activates below 620 px; visual verification at minimum size remains | in progress |
| GUI-002 | Accurate empty, loading, rendering, ready, error, and missing-source states | `MainComponent.cpp` | State paths and actionable messages are implemented; manual UI walkthrough remains | in progress |
| GUI-003 | Mode, macros, advanced controls, and actions update parameters | `MainComponent.cpp`, `AdvancedPanel.cpp` | Wired to render, preview, preset, undo, reset, and export paths; manual interaction check remains | in progress |
| GUI-004 | Accessible labels, focus, numeric entry, scaling, contrast, and tooltips | `MainComponent.cpp`, `AdvancedPanel.cpp` | Help and global tooltip toggle are present; focus, accessibility, and scale checks remain | in progress |
| APP-001 | Windows/macOS standalone target builds and launches without plug-in targets | `CMakeLists.txt`, `.github/workflows/build.yml` | macOS compile/tests passed on prior tree; latest cross-platform build is pending; launch smoke tests remain | in progress |
| APP-002 | Source selection, metadata, analysis, waveform, invalid replacement preservation | `SourceLoader.cpp`, `SourceAnalyzer.cpp`, `MainComponent.cpp` | Native file and analyzer tests; latest extension guard is pending CI | in progress |
| APP-003 | Snare/Clap mode and connected workflow actions | `MainComponent.cpp` | Native engine tests plus compile coverage; UI walkthrough remains | in progress |
| HOST-001 | VST3/CLAP/AU targets after standalone acceptance | Future plug-in targets | Explicitly deferred; no plug-in format is built in this phase | pending |
| HOST-002 | MIDI notes, velocity, note-off, overlapping plug-in voices | Future plug-in playback | Explicitly deferred with plug-in formats | pending |
| HOST-003 | Host automation and callback-safe generation | Future plug-in processor | Explicitly deferred with plug-in formats | pending |
| HOST-004 | Host-project state persistence | Future plug-in processor | Standalone recovery exists; host-project persistence is deferred | pending |
| IO-001 | Validated JSON user and factory presets | `PresetManager.cpp`, `FactoryPresets.cpp` | Preset round-trip and factory bounds/unique-name tests are present | in progress |
| IO-002 | Bounded randomize/mutate and exact settings undo/reset | `SnairEngine.cpp`, `MainComponent.cpp` | Native tests cover bounds and mode preservation; UI sequence check remains | in progress |
| IO-003 | WAV export at supported depths, rates, channels; validate before replace | `WavExporter.cpp` | PCM16/24 and float32 metadata/resample tests; invalid rate/trim rejection tests; latest cross-platform run pending | in progress |
| QA-001 | Automated native build and test gates | `native/tests/EngineTests.cpp`, CI workflow | macOS build/tests passed on prior tree; latest Windows/macOS verification pending | in progress |
| QA-002 | Real source corpus, listening, and platform smoke checks | Release QA record | Not performed in CI | pending |
| QA-003 | End-to-end standalone source→render→edit→preview→save/reopen→WAV | Release QA record | Components and native IO tests exist; complete manual workflow evidence remains | pending |

Do not mark a release check verified without its corresponding evidence.