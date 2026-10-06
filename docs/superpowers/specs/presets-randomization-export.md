# SnairCreator Presets, Variation, and WAV Export Specification

## Presets

Factory presets are parameter recipes, not copyrighted sample content. A preset may reference source analysis; it must name whether source data is embedded, external, or missing. User presets store complete parameter/seed state and, where needed, rendered-hit recovery data. Preset load runs validation and schedules generation off-thread. Invalid fields use documented defaults only with a visible warning; malformed preset cannot reset unrelated state.

Support Save, Save As, Load, and a preset selector as approved by design. Keep preset save distinct from DAW project state and WAV export. No preset action changes the current source without showing that it will be replaced.

## Randomize

Randomize makes a meaningfully different but bounded variation. It may change seed, extraction positions, body frequency, pitch, texture, tone, drive, size and mode-relevant burst settings. Values are clamped to the approved parameter bounds. It must not alter mode unless explicitly included in a future approved behavior; default preserves current Snare/Clap mode. Show Rendering, retain prior hit, and publish only latest valid result. Same starting state and randomize seed policy must be deterministic for reproducible undo/redo.

## Mutate and Undo

Mutate creates a smaller change, typically within the design's 10–20% normalized parameter movement target, preserving mode and source identity. Discrete parameters may move one valid step. Save pre-mutation parameters, seed and render identity. Undo restores that state and prior sound instantly when cached, or deterministically rerenders it off-thread. At least one-step undo is mandatory. An active render cannot overwrite the undo snapshot until completed state is committed.

## Reset

Reset restores public parameters and seed defaults according to the design, but does not delete source file, factory/user presets, or unrelated host state. Confirm only if user-created preset data would be removed; ordinary parameter reset should not be destructive.

## WAV export

Export writes the exact current immutable RenderedHit used by Preview/MIDI, not a fresh randomized or separately rendered sound. The UI reports whether a pending render means visible controls differ from the currently active hit; disable export or label exported active generation until latest render finishes.

Required output: mono/stereo, 16-bit PCM, 24-bit PCM, 32-bit float. Sample rates: current render/host rate, 44.1, 48, 88.2, 96 kHz. Use quality offline resampling if rates differ. Options include normalize, include output trim, channel count, bit depth and rate. Clarify output-trim policy and keep export options out of host automation.

Filename default follows approved format SnairCreator_Mode_Source_Seed.wav. Sanitize invalid characters, avoid overwrite without consent, write temporary then finalize, reopen/validate file, and show location/duration/rate/depth on success. Failure leaves hit/state unchanged and reports destination/encoding issue.

## Acceptance criteria

- Same preset/state restores source identity, parameters and recoverable sound.
- Randomize stays in legal bounds and produces measurable variation.
- Mutate is smaller than Randomize and Undo restores previous audible result.
- Preview, MIDI and export fingerprints identify the same generation.
- Export tests reopen every bit depth/rate/channel format and verify duration/peak/sample equivalence within encoding/resampling tolerance.
- Cancellation/overwrite/export failure cannot corrupt an existing file or plugin state.
