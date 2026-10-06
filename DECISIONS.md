# SnairCreator Decisions

## D-001 Approved design is product authority
The approved design specification at docs/superpowers/specs/2026-10-03-snaircreator-design.md defines product scope and behavior. The dated implementation plan is subordinate when details conflict. Focused specs expand the approved design.

## D-002 Editor size
Use the approved design dimensions: default 1000 × 700; minimum 760 × 520. The implementation plan's GUI task lists alternate dimensions; update that task before implementing the editor.

## D-003 Platform scope
First-class targets are Windows VST3, CLAP and Standalone; macOS VST3, CLAP, AU and Standalone. Linux is optional after stable CI; AAX is out of scope. Follow approved design section 28.

## D-004 Build baseline
Use C++20, JUCE 8.x, CMake 3.24+, pinned dependencies and native automated tests as specified by the implementation plan. Resolve exact JUCE and CLAP-extension revisions/provisioning before the first reproducible CI build.

## Open decisions
- Choose the precise pinned JUCE and clap-juce-extensions revisions and license/redistribution review.
- Decide how much rendered audio is serialized into DAW state and set a maximum host-state size.
- Choose the silence-source policy (procedural fallback versus require a new source) consistently before implementing generation.
- Select supported extra codecs only after platform availability and distribution terms are reviewed.
- Determine signing/notarization process for shipped binaries.
