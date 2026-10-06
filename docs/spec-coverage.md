# SnairCreator Specification Coverage

Standalone-first implementation is underway. Do not mark an item verified without implementation location and concrete evidence. Numerical tests do not replace listening or platform validation.

| ID | Source | Acceptance / verification | Implementation location | Evidence | Status |
|---|---|---|---|---|---|
| BUILD-001 | DECISIONS D-001–D-004 | Clean C++20/CMake/JUCE 9.0.3 standalone build with pinned dependency | CMake/CI | Not implemented | pending |
| SRC-001 | source-ingestion.md, Ingestion lifecycle | WAV/AIFF decode via chooser and drag-drop with preserved prior source on failure | SourceLoader/UI | Not implemented | pending |
| SRC-002 | source-ingestion.md, Edge behavior | Silence/short/stereo/nonfinite/long/malformed inputs safely handled | analyzer tests | Not implemented | pending |
| SRC-003 | source-ingestion.md, Threading and memory | File IO/decode/analysis absent from realtime callback | source worker | Not implemented | pending |
| DSP-001 | render-engine.md, Core data contracts | Versioned analysis/render contracts include source/parameter/seed/request identity | DSP model | Not implemented | pending |
| DSP-002 | render-engine.md, Snare behavior | Source-derived attack/body/texture/reinforcement produce bounded finite render | SnareRenderer | Not implemented | pending |
| DSP-003 | render-engine.md, Clap behavior | 2–6 deterministic bursts and texture/tail respond to count/spread | ClapRenderer | Not implemented | pending |
| DSP-004 | render-engine.md, Determinism and request queue | Same state/seed reproduces; stale jobs coalesced/discarded; old hit remains active | RenderController | Not implemented | pending |
| DSP-005 | render-engine.md, Output safety | Nonfinite/out-of-range output rejected or bounded; edge fades and peak rules pass | safety stage | Not implemented | pending |
| GUI-001 | gui-implementation.md, Layout contract | Approved 1000×700 default and 760×520 minimum; resizable editor | PluginEditor | Not implemented | pending |
| GUI-002 | gui-implementation.md, UI states | Empty/loading/analyzing/ready/rendering/error/missing-source states match contract | UI components | Not implemented | pending |
| GUI-003 | gui-implementation.md, Interaction wiring | Mode/macros/advanced controls update stable parameter state | editor/controller | Not implemented | pending |
| GUI-004 | gui-implementation.md, Accessibility | Labels, focus, numeric entry, scaling and contrast verified | UI | Not implemented | pending |
| APP-001 | standalone-first.md, Application shell | Windows standalone target configures, builds, launches, resizes, and exposes no plugin formats | native app target | Shell source added; build not run | in progress |
| APP-002 | standalone-first.md, Source selection | WAV/AIFF chooser/drop validates file and displays metadata; invalid replacement preserves prior source | app source panel | Metadata-only shell added; decode tests pending | in progress |
| APP-003 | standalone-first.md, Mode and actions | Snare/Clap selection works; unimplemented actions remain disabled | app GUI | Shell source added; UI test pending | in progress |
| HOST-001 | plugin-host-state.md, Targets and formats | VST3/CLAP/AU targets are added only after standalone acceptance | future plugin targets | Deferred | pending |
| HOST-002 | plugin-host-state.md, MIDI playback | Note, velocity, note-off and overlapping voices behave correctly | playback engine | Not implemented | pending |
| HOST-003 | plugin-host-state.md, Host automation | Automated generation stays off callback; latest render safely publishes | processor/render control | Not implemented | pending |
| HOST-004 | plugin-host-state.md, Session persistence | Project restore preserves hit with original source present and missing | state serializer | Not implemented | pending |
| IO-001 | presets-randomization-export.md, Presets | Preset load validates state/source and preserves unrelated state | preset service | Not implemented | pending |
| IO-002 | presets-randomization-export.md, Randomize / Mutate / Undo | Bounded variations and exact prior-state restoration | variation controller | Not implemented | pending |
| IO-003 | presets-randomization-export.md, WAV export | All bit depths/rates/channels export current hit and validate after write | WavExporter | Not implemented | pending |
| QA-001 | verification-build-release.md, Automated test layers | Unit/integration/UI suites pass on clean supported builds | tests/CI | Not implemented | pending |
| QA-002 | verification-build-release.md, Manual source and host matrix | Real DAW and source corpus acceptance performed and recorded | QA records | Not implemented | pending |
| QA-003 | verification-build-release.md, Release acceptance | End-to-end source→render→edit→MIDI→save/reopen→WAV succeeds | release audit | Not implemented | pending |

Statuses: pending, in progress, verified, blocked. Record evidence and build/host details when changing status. Do not claim completion while required release checks remain open.
