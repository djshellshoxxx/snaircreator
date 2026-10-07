# SnairCreator v0.0.1 beta

The first public beta of SnairCreator by Circuit Drift Labs. It turns any WAV/AIFF sound into a playable snare or clap.

## Downloads

| Platform | Installer | Portable | Plug-ins |
|---|---|---|---|
| Windows x64 | `...-Windows-Installer.exe` (Standalone/VST3/CLAP, selectable) | `...-Windows-Portable.zip` | `...-Windows-VST3.zip`, `...-Windows-CLAP.zip` |
| macOS (Apple Silicon + Intel) | `...-macOS-Installer.pkg` | `...-macOS-Portable.zip` | `...-macOS-VST3.zip`, `...-macOS-CLAP.zip`, `...-macOS-AU.zip` |
| Linux x64 | – | `...-Linux-Portable.tar.gz` | `...-Linux-VST3.tar.gz`, `...-Linux-CLAP.tar.gz` |

## Highlights

- Snare and Clap engines that use the source: transient crack, source-excited shell resonance, grain-scrambled wire/clap texture. Source Character controls how much of the source survives.
- Six macros plus the full advanced parameter set, with stable automation IDs.
- MIDI and Preview play the same rendered buffer: 16 voices, velocity, note-off never cuts.
- Randomize, Mutate, 16-level Undo and Reset. Renders are deterministic per source, settings and seed.
- Exact-render WAV export, presets, and session recall with the rendered hit embedded.
- **New:** drag-out of the hit to your DAW, Export Kit (8-sound kit), Key Track chromatic play, and a Gated Room effect.

## Known limitations

- Builds are unsigned (Windows SmartScreen, macOS Gatekeeper: right-click > Open).
- Automated validation ran on Linux (pluginval strictness 8, clap-validator). Hands-on DAW testing on Windows and macOS is still pending, so please report host issues.
- Host automation renders in the background, so an offline bounce can play the previous hit until the new render finishes (usually a few milliseconds).
- Seed automation is exact only below 16,777,216, a limit of float parameters. Generated seeds always stay below 1,000,000.
- Input formats: WAV and AIFF only.
