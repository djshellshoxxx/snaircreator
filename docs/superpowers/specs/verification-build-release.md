# SnairCreator Verification, Build, and Release Specification

## Scope

The approved design is authoritative for platform scope, manual QA and acceptance. The existing implementation plan is task ordering. Do not claim beta readiness from compilation alone.

## Build baseline

C++20, JUCE 8.x with pinned revision, CMake 3.24+, VST3 and CLAP on Windows/macOS, AU on macOS, Standalone. CLAP integration is pinned and reviewed. AAX is excluded; Linux is optional. CI must build native tests before publishing artifacts.

Expected clean build after scaffold:

    cmake -S . -B build -DSNAIRCREATOR_BUILD_TESTS=ON
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure

Build instructions must state compiler, JUCE provisioning, CLAP extension revision, platform SDK and artifact locations. Do not rely on user-local JUCE paths.

## Automated test layers

1. Source decode/analysis: silence, impulse, sine, noise, short, stereo, non-finite, late transient.
2. Parameter model: stable IDs, valid ranges, invalid-value sanitation, mode-specific availability.
3. Determinism: same input/state/seed equivalent; changed seed differs; Randomize bounded; Mutate bounded; Undo restores.
4. DSP render: finite/peak-bounded output, attack/body/tail and duration constraints, Clap burst count/spread, no out-of-range reads.
5. Playback: note trigger, velocity, 16 voices, note-off behavior, buffer swap and sample-rate change.
6. State: parameter/mode/seed/source fingerprint restore, rendered sound restore with missing source, malformed/oversized state.
7. Export: WAV PCM16/PCM24/float32, mono/stereo, all rates, current-render identity, overwrite/cancel/failure.
8. GUI: editor instantiation, file chooser/drop, parameter wiring, Advanced toggle, preview/randomize/mutate/undo/reset, render/error statuses and export gating.
9. Stress: rapid automation/MIDI/source changes, render cancellation, large files, repeated open/close, memory/queue bounds.

Numerical tests validate measurable behavior; subjective snare/clap quality requires listening review and a recorded source/mode/parameter result.

## Manual source and host matrix

Source classes: kick, vocal, guitar, sustained synth, noise, metallic impact, field recording, music excerpt, very quiet source and very short transient. Exercise Snare and Clap; load/drop; Preview/MIDI; parameter/automation changes; Randomize/Mutate/Undo; preset save/load; project save/reopen; missing source; WAV export.

At least one major Windows DAW and one macOS DAW for broadly usable beta. Validate VST3/CLAP on both target OSs and AU on macOS. Standalone must independently load source, render, preview and export. Linux optional and cannot be claimed supported until separately built/tested.

## Release acceptance

- Clean Release configure/build on required OS targets.
- Automated tests pass.
- Actual host validation for VST3 and CLAP; AU on macOS; standalone smoke test.
- Load → analyze → Snare/Clap render → edit → Preview/MIDI → save/reopen → WAV export succeeds.
- Five materially different source classes per mode pass finite/peak/character checks and listening review.
- Rapid parameters and MIDI do not interrupt callback.
- Reopened project restores hit with and without original source.
- WAV exports reopen at all required formats and match active hit.
- No unresolved critical security, state-loss, audio-thread or false-success defect.
- docs/spec-coverage.md has no required item marked pending/in progress/blocked for the release claim; blocked external-host checks remain honestly disclosed.
- Release notes, install guide, license notices and dependency attributions are present.
