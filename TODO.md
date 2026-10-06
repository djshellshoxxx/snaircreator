# SnairCreator TODO

## Current status
Design and implementation plan are approved; no implementation source was present during repository audit. Focused implementation specifications and coverage tracking are now linked. All implementation/host checks remain pending.

## Next work
1. Reconcile the implementation plan's editor dimensions with DECISIONS.md.
2. Pin JUCE and CLAP extension versions and document reproducible provisioning/licensing.
3. Scaffold CMake targets and native tests.
4. Implement stable parameter model and deterministic RNG.
5. Implement WAV/AIFF source loader and analyzer.
6. Implement offline Snare and Clap renderers with safe source fallbacks.
7. Add realtime playback/MIDI and background render controller.
8. Add persistent host state, presets and missing-source recovery.
9. Add exact-current-render WAV export.
10. Build the GUI and wire every action/control.
11. Add CI, docs, real-host validation and a full coverage audit.

## Release blockers
- No source implementation yet.
- Dependency revisions and provisioning are not pinned.
- Silence source behavior is undecided.
- Host-state size limit and rendered-hit persistence strategy need a decision.
- Windows/macOS hosts and listening validation are not yet evidenced.
