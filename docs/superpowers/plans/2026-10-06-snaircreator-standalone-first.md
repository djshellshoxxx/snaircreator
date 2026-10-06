# Standalone-First Implementation Plan

This plan supersedes the plugin-first delivery order in the 2026-10-04 plan. The earlier plan remains a backlog reference for plugin-specific work after standalone acceptance.

## Slice 1: Native app shell and GUI
- Pin JUCE 9.0.3 through CMake FetchContent.
- Add a standalone JUCE GUI target only.
- Implement application startup/window lifecycle, responsive layout, source chooser/drop, file metadata display, Snare/Clap selection, and visible disabled states for unfinished DSP actions.
- Verify source syntax/configuration and run clean Windows/macOS builds when runners are available.

## Slice 2: Source ingestion
- Decode WAV/AIFF off the UI/audio callback.
- Preserve existing source on failed replacement.
- Add validation, bounded memory/duration handling, analysis representation, waveform view, and tests for malformed/silent/short/stereo/long input.

## Slice 3: First render path
- Implement deterministic source analysis and a safe Snare render.
- Add immutable render data and a worker controller.
- Verify finite bounded output, determinism, cancellation, and edge fades.

## Slice 4: Standalone playback
- Add audio-device setup and safe preview playback.
- Preview must play the same immutable render used by export.
- Verify start/stop/retrigger/overlap behavior and device loss/recovery.

## Slice 5: Clap and controls
- Implement deterministic 2–6 burst Clap rendering.
- Wire macros and advanced controls to render requests; coalesce stale work.

## Slice 6: Export and persistence
- Export the exact current hit as WAV in requested sample formats.
- Add presets, randomize/mutate/undo, recovery, and documented limits.

## Slice 7: Standalone release validation
- Add Windows and macOS CI.
- Run automated tests and manual load → render → preview → export acceptance.
- Update coverage with implementation locations and evidence.

## Deferred format work
Only after standalone acceptance, add VST3, CLAP, and AU wrappers/build targets. Keep plugin host state, DAW automation, MIDI note triggering, and real-host validation outside the standalone shell slices.
