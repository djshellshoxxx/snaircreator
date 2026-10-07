# Native source

One JUCE `AudioProcessor` (`PluginProcessor`) and editor (`PluginEditor`) serve every format (Standalone, VST3, CLAP, AU).

- Analysis/render (no UI, no audio thread): `SourceLoader`, `SourceAnalyzer`, `LayerExtraction`, `SnareRenderer`, `ClapRenderer`, `SnairEngine`, `DeterministicRng`
- Realtime playback: `HitPlayer`
- Persistence and IO: `PresetManager`, `FactoryPresets`, `WavExporter`, and processor state
- UI: `PluginEditor`, `WaveformView`
- Tests: `tests/EngineTests.cpp`

See [BUILDING.md](../BUILDING.md).
