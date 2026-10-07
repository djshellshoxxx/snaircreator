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

## D-006 Plug-in formats delivered with v0.0.1 (supersedes the deferral in D-005)
The standalone workflow is now feature-complete, so the plug-in formats ship in the same beta. Standalone, VST3 and CLAP target Windows/macOS/Linux; AU targets macOS. All formats share one `AudioProcessor`/editor, and the standalone uses JUCE's standalone wrapper for audio/MIDI device setup and state persistence. CLAP uses clap-juce-extensions pinned to commit 7adee3a, the first revision that supports JUCE 9.

## D-007 Rendered hit embedded in state
Host state and the standalone session embed the current rendered hit as float32 data (at most 3 s at 192 kHz stereo). The source path and fingerprint are stored for re-editing only. If the source on disk changed (fingerprint mismatch), it is not substituted silently.

## D-008 Silence policy
Silent or effectively silent sources are rejected with an actionable message. Generation needs audible material.

## D-009 Output trim policy
Output trim is applied at playback, smoothed per block. Export applies it only when "Include output trim" is checked. Trim changes never re-render.

## D-010 Seed parameter
`seed` keeps the spec range 0–2147483647 but is float-backed, which avoids int overflow in host conversions. Randomize/Mutate generate seeds of at most 999,999, which survive float automation exactly.

## D-011 v0.0.1 feature additions
Drag-out (usability), Export Kit (value), Key Track via the new `key_track` parameter (fun) and Gated Room via the new `room` parameter (random effect). Both new IDs are stable from v0.0.1.

## Open decisions
- Select supported extra codecs only after platform availability and distribution terms are reviewed.
- Determine signing/notarization process for shipped binaries.

