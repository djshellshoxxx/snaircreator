# SnairCreator Plugin, MIDI, Automation, and Host State Specification

## Targets and formats

First-class targets are Windows VST3, CLAP, Standalone; macOS VST3, CLAP, AU, Standalone. Linux is optional after CI proves stable; AAX is out of scope. JUCE/C++ and CMake are required; pin dependencies and use clap-juce-extensions only if its maintenance/license status is acceptable.

The standalone build uses the same DSP/editor components but obtains audio/MIDI device setup from its standalone wrapper. Plugin builds receive audio/MIDI from the host. The instrument must not assume an incoming audio input.

## MIDI playback

- MIDI note-on triggers current RenderedHit; velocity scales amplitude.
- Note-off does not cut a normal one-shot by default.
- Support overlapping one-shots up to 16 voices by default; enforce voice cap predictably.
- Accept any note by default. C1/MIDI 36 and D1/MIDI 38 may be documented convenience defaults, not required notes.
- Preview triggers the exact same hit and voice path without requiring MIDI.
- Rapid MIDI while render changes must remain glitch-free; an active voice may finish using its immutable buffer while future notes use the newly published generation.

## Host automation

Stable public parameter IDs are required for mode, Source Character, six macros, body frequency, attack/noise/tail/pitch/tone/drive/width/clap count/spread/cross blend/output trim/normalization/seed where appropriate. UI and host automation share one parameter model. Automation must not cause offline rendering in the host callback.

When an automated parameter needs regeneration:
1. capture a consistent parameter snapshot;
2. coalesce obsolete requests;
3. render off realtime thread;
4. keep old hit active;
5. atomically publish only matching newest render;
6. display Rendering and completion/error state.
Export-specific format/bit-depth/sample-rate choices need not be host automated.

## Session persistence

Persist public parameter state, mode, seed, source metadata/fingerprint, and a reliable representation of the current generated hit. Reopening the host project must restore sound even if original source file moved. Source path is for re-edit/reanalysis only; if missing, report it without replacing sound. Host state size must be bounded; prefer embedding rendered hit and omit full source when size policy requires. The user can relink source to regenerate.

Preset state and host project state are distinct: a preset may reference a source and warn if unavailable; host project must restore current hit. Save/reload must not fire preview or audio output by itself.

## Sample rate and host lifecycle

- Prepare/release/reset follows JUCE host lifecycle and does not block audio callback.
- If host sample rate changes, existing rendered hit must play at correct pitch/time or schedule a converted re-render off-thread.
- Buffer-size changes cannot invalidate playback or allocate source-sized memory on callback.
- Suspend/restore and plugin editor close/reopen do not lose processor state.
- Processor without source or render outputs silence, not noise/NaN.
- Host bypass and transport behavior remain controlled by host; plugin does not infer tempo for first beta.

## Acceptance criteria

- Validate VST3 and CLAP in actual compatible hosts; AU on macOS.
- MIDI note/velocity/overlap/note-off behavior matches contract.
- Host automation changes displayed values and generates safely off-thread.
- Save/reopen restores parameters and exact hit with source present and missing.
- Sample-rate/buffer changes remain finite and artifact-free.
- Standalone opens and supports audio/MIDI setup through shared product controls.
