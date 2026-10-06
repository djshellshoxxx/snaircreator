# SnairCreator TODO

## Current status
Standalone-first implementation has started. The first code slice adds a JUCE desktop application shell and a responsive source/mode GUI. Loading currently identifies supported audio files and shows metadata; audio transformation, preview playback, presets, and WAV export are not implemented yet.

## Next work
1. Build and launch the standalone shell on Windows; add macOS build verification.
2. Implement bounded WAV/AIFF decoding, source ownership, and waveform display.
3. Implement deterministic source analysis and the first Snare render path.
4. Implement Clap rendering, then safe preview playback.
5. Wire controls to the render engine and enable actions only when valid output exists.
6. Add WAV export of the exact current render.
7. Add presets, randomize/mutate/undo, and standalone project/session recovery.
8. Add platform CI and a requirement coverage audit.
9. After the standalone workflow is stable, add VST3, CLAP, and AU as separate delivery work.

## Release blockers
- Native builds and GUI behavior have not yet been verified on Windows or macOS.
- Audio decoding, analysis, sound generation, playback, export, and persistence remain incomplete.
- JUCE licensing/distribution terms must be checked for the intended release model before shipping.
