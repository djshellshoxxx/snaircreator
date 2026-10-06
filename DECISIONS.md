# SnairCreator Decisions

## D-001 Approved design is product authority
The approved design specification at docs/superpowers/specs/2026-10-03-snaircreator-design.md defines product scope and behavior. Focused specifications expand the approved design. The standalone-first specification controls delivery order and format scope where older plugin-first documents conflict.

## D-002 Editor size
Use the approved design dimensions: default 1000 × 700; minimum 760 × 520.

## D-003 Platform scope
The first standalone release targets Windows and macOS. Linux is optional after stable CI. Build and validate the desktop standalone app before adding plugin formats.

## D-004 Build baseline
Use C++20, JUCE 9.0.3, CMake 3.24+, and pinned dependencies. JUCE licensing and distribution terms must be reviewed for the intended release model. Do not vendor JUCE or imply that the repository's proprietary license covers JUCE.

## D-005 Standalone first
The first deliverable is the standalone desktop application. VST3, CLAP, and AU targets are deferred until the standalone workflow is stable. Keep sound-generation and audio-processing code independent of the application shell so later formats can reuse it; do not add plugin targets or plugin-specific code in this first implementation slice.

## Open decisions
- Decide whether rendered audio is serialized into DAW state after plugin formats are scheduled.
- Select supported extra codecs only after platform availability and distribution terms are reviewed.
- Determine signing/notarization process for shipped binaries.
- Define the silence-source policy before implementing generation.

