# Standalone-First Specification

## Product delivery order
SnairCreator begins as a native desktop application. The first release format is Standalone. VST3, CLAP, and AU are later targets and must not block the standalone application.

The first platform targets are Windows and macOS. Linux may follow after stable CI. The sound-generation core must remain independent of the desktop window and later plugin wrappers.

## First implementation slice
The first slice provides a buildable JUCE GUI application with:
- resizable editor, default 1000 × 700 and minimum 760 × 520;
- source file chooser and drag-and-drop for one audio file;
- WAV/AIFF reader validation and source filename, duration, sample rate, and channel display;
- Snare/Clap mode selection;
- Source Character and six macro controls shown in their approved positions;
- visible workflow states and accurate disabled states for features not implemented;
- no claim that source loading has transformed or auditioned the sound.

This slice is the application shell. It does not implement source decoding into persistent project memory, analysis, transformation, playback, randomization, presets, or export.

## Standalone workflow milestones
1. Load and decode one valid WAV/AIFF file without losing the previous valid source on failure.
2. Analyze the source asynchronously and display its waveform and candidate transients.
3. Generate a deterministic Snare render and expose a safe Preview action.
4. Add Clap generation and make mode-specific controls behave as specified.
5. Add WAV export of the exact currently previewed render.
6. Add recovery/preset behavior and complete acceptance tests.
7. Validate the application on Windows and macOS.
8. Schedule VST3, CLAP, and AU only after the standalone workflow is stable.

## GUI state rules
- Empty state: invite the user to load a source; generation and export actions are disabled.
- Source selected but not rendered: show source metadata; preview of generated hit, randomize, mutate, undo, and export remain disabled.
- Rendering: show progress without freezing the window; retain the previous valid render if one exists.
- Ready: Preview and Export refer to the same immutable render.
- Error: show a specific, actionable message and preserve the last valid source/render.
- Unimplemented controls must be visibly disabled or clearly labeled; they must not imply that DSP is connected.

## Architecture
Use JUCE for the first desktop application shell. Keep the future DSP/source model in app-independent C++ modules. Standalone-only startup, windowing, file chooser, and device configuration belong to the app layer. Do not create plugin processors, plugin format targets, host automation, or DAW state code in this phase.

## Acceptance for the shell
- CMake configures from a clean checkout with the pinned JUCE revision.
- Windows standalone target compiles and launches.
- The app window resizes without overlapping controls at minimum size.
- Snare/Clap selection visibly updates the selected mode.
- WAV/AIFF selection or drop displays correct file metadata; invalid input reports an error.
- Actions without an implementation remain disabled and accurately described.
- No VST3/CLAP/AU target is produced by the initial build.
