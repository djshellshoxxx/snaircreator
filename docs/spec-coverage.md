# SnairCreator Specification Coverage Audit (v0.0.1 beta)

Audit of the implementation against the [approved design](superpowers/specs/2026-10-03-snaircreator-design.md) and the focused specs. "Verified" means an automated test or validator ran and passed on Linux. External DAW checks on Windows/macOS are listed separately and honestly as pending.

| ID | Requirement | Implementation | Evidence | Status |
|---|---|---|---|---|
| BUILD-001 | C++20/CMake/JUCE 9.0.3, pinned deps; VST3, CLAP, Standalone (+AU on macOS) | `CMakeLists.txt` | Linux build of all targets; CI matrix Win/macOS/Linux | Verified (Linux), CI for Win/mac |
| SRC-001 | WAV/AIFF via chooser and drop; keep previous source on failure | `SourceLoader`, `PluginProcessor::loadSource`, editor drop | IO tests (WAV, AIFF, corrupt, disguised) | Verified |
| SRC-002 | Silence, short, stereo, non-finite, long, size bounds | `SourceLoader`, `SourceAnalyzer`, `LayerExtraction` | Analyzer and engine tests; 10 min / 256 MiB limits | Verified |
| SRC-003 | Decode/analysis/render never on the audio callback | `ThreadPool` job + render `Worker` thread | Processor tests; callback reads immutable buffers only | Verified |
| ANA-001 | SourceAnalysis descriptors (§5) incl. late transient, fingerprint | `SourceAnalyzer.cpp` | Late-transient, impulse/tone/noise tests | Verified |
| DSP-001 | Snare: attack, modal + source-excited body, wire, reinforcement | `SnareRenderer.cpp` | Finite/peak/attack/macro-response tests | Verified numerically; listening review recommended |
| DSP-002 | Clap: 2–6 bursts, 8–35 ms spacing, deterministic jitter, tail + diffusion | `ClapRenderer.cpp`, `addBursts` | Burst count/spacing/determinism tests | Verified |
| DSP-003 | Source Character influences several stages (not dry/wet) | `LayerExtraction`, both renderers | Character extremes test | Verified |
| DSP-004 | Determinism across platforms; seed changes output | `DeterministicRng` (SplitMix64, no std distributions) | Determinism tests | Verified |
| DSP-005 | Output safety: finite, edge fades, peak ≤ 0.98 (normalized or not) | `SnairEngine::applySafety` | Every render test asserts finite + peak | Verified |
| DSP-006 | Cross blend; tone, drive, width, pitch, tail mappings | `SnairEngine`, renderers | Macro/advanced response test | Verified |
| PLAY-001 | MIDI note-on, velocity, 16 voices, note-off no cut, sample-accurate | `HitPlayer`, `processBlock` | Playback and processor tests | Verified |
| PLAY-002 | Preview uses the same hit as MIDI/export | `triggerPreview` → same `HitPlayer` | Design: one active hit pointer | Verified |
| PLAY-003 | Old hit plays during render; safe atomic swap; coalesced requests | `publish`, `collectGarbage`, `Worker` | Re-render and coalescing tests | Verified |
| PLAY-004 | Sample-rate change: correct pitch until re-render | rate ratio in `HitPlayer`, re-render on prepare | Key-track rate test | Verified |
| VAR-001 | Randomize bounded, keeps mode; Mutate ≤15% moves; 16-level Undo restores hit | `SnairEngine::randomized/mutated`, `undo` | 200-iteration bounds test; undo restores exact hit | Verified |
| VAR-002 | Reset restores defaults without touching source (undoable) | `resetParameters` | Manual | Implemented |
| EXP-001 | WAV 16/24/32f, mono/stereo, render/44.1/48/88.2/96 kHz, validated atomic write | `WavExporter` | Export matrix test reopens every file | Verified |
| EXP-002 | Filename `SnairCreator_<Mode>_<Source>_<Seed>.wav`, sanitized | `defaultExportName` | Manual | Implemented |
| STATE-001 | Session saves params, seed, source path/fingerprint, embedded hit | `get/setStateInformation` | Round-trip test with source deleted | Verified |
| STATE-002 | Malformed/future state ignored safely; missing source reported | `setStateInformation`, status text | Malformed + future state test | Verified |
| PRE-001 | Factory recipes, Save/Save As/Load, selector, validation with warnings | `FactoryPresets`, `PresetManager`, editor | Preset round-trip and invalid-field tests | Verified |
| GUI-001 | 1000×700 default, 760×520 min, resizable, no overlap | `PluginEditor::resized` | Snapshot review at both sizes | Verified |
| GUI-002 | Empty/loading/rendering/ready/error/missing-source states, stale-hit notice | `refresh`, processor status | Snapshot + processor tests | Verified |
| GUI-003 | All controls bound to stable parameter IDs (no dead controls) | APVTS attachments | ID test; pluginval/clap-validator param checks | Verified |
| GUI-004 | Tooltips + global toggle, help, double-click reset, context menu value entry, fine wheel, focus outline, keyboard shortcuts | `FineSlider`, `SnairLookAndFeel`, editor | Manual | Implemented |
| GUI-005 | Waveform marks strongest transient and attack region; generated-hit toggle | `WaveformView` | Snapshot | Verified |
| NEW-001 | Drag hit out to DAW (usability) | `WaveformView::onDragOut`, `writeDragFile` | Manual (needs a desktop session) | Implemented |
| NEW-002 | Export Kit, 8 variations (value) | `exportKit` | Manual | Implemented |
| NEW-003 | Key Track chromatic playback (fun) | `HitPlayer::noteOn`, `key_track` | Playback test | Verified |
| NEW-004 | Gated Room effect (random effect) | `applyGatedRoom`, `room` | Macro-response test | Verified |
| VAL-001 | Format validation | — | pluginval strictness 8 SUCCESS; clap-validator 18/18 | Verified (Linux) |
| HOST-001 | Real-DAW checks on Windows and macOS; AU validation (auval) | — | Not run in this environment | Pending (beta disclosure) |

## CDL baseline compliance

| Area | Status |
|---|---|
| Profiles | Instrument plug-in + standalone companion + offline exporter |
| 2.1–2.3 Controls, parameter contract, gestures | Implemented (stable IDs, units, double-click reset, context menu, fine wheel) |
| 2.4 Accessibility | Implemented: text-paired states, focus outlines, keyboard shortcuts. No essential animation. |
| 3.2 Real-time safety | Implemented: no locks, allocation or IO in `processBlock`; deferred release protocol |
| 3.3 Buses | No input, mono or stereo output. Latency 0. Tail 3 s. |
| 3.4 Instrument behavior | Silent with no hit. Note-off ignored. All-notes-off / all-sound-off stop voices. |
| 4.1 Host state | Implemented, versioned (`version` = 1) |
| 4.2 Presets | Implemented: JSON, validated, atomic write |
| 4.3 Export/file loading | Implemented (formats and limits documented in the in-app help) |
| 5 MIDI | Notes + velocity on any channel; no MIDI Learn (not applicable) |
| 6 Visual identity | Implemented (CDL palette; cyan/magenta product accents per design §21) |
| 7 Help/About | In-app help with version, usage, formats and CDL link |
| 8 Privacy | No network, telemetry or licence checks |
| 9 Quality gates | Automated tests and validators recorded above; host matrix pending |

**Status: BETA READY** for the automated scope. Outstanding: hands-on DAW validation on Windows/macOS, auval, and a listening review of the source corpus.
