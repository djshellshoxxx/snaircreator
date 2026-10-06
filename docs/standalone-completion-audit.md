# Standalone Completion Audit

This audit distinguishes repository implementation from platform validation.

## Implemented
- bounded WAV/AIFF decode with prior-source preservation on failure
- waveform extraction and display
- deterministic source analysis with transient scan, level, crest, spectral and source fingerprint descriptors
- deterministic Snare and Clap render paths
- finite-sample sanitation, edge fades, peak protection and optional render normalization
- Source Character plus six primary macros
- Preview playback of the exact active RenderedHit
- Randomize, Mutate, one-level Undo and Reset
- JSON user preset save/load
- WAV export from the exact active render
- Windows/macOS CMake workflow and native engine tests
- standalone-only delivery remains separate from deferred plug-in formats

## Engineering decisions
The first standalone renderer uses source-derived transient/body/texture material plus deterministic procedural reinforcement. Source Character simultaneously changes source contribution and reinforcement rather than behaving as dry/wet. Snare body resonance is constrained to 70–450 Hz and informed by the analyzed source. Clap uses 2–6 deterministic micro-bursts with bounded 8–35 ms spacing. The active render remains immutable during Preview/export.

## Validation still requiring external runners
A source repository cannot truthfully self-certify listening quality, device compatibility, or launch behavior on every Windows/macOS system. GitHub Actions is the automated build/test gate; manual listening and platform smoke tests remain release evidence rather than missing implementation.
