# Building SnairCreator

## Requirements

- CMake 3.24+, a C++20 compiler, Git
- Windows: Visual Studio 2022+ (Desktop C++). macOS: Xcode 15+. Linux: GCC 11+/Clang 14+ and
  `libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxcomposite-dev libxext-dev libxi-dev libxss-dev libxtst-dev libxrender-dev libgl1-mesa-dev libfreetype-dev libfontconfig1-dev libjack-jackd2-dev`

CMake fetches pinned dependencies during configuration:

| Dependency | Revision | Licence |
|---|---|---|
| JUCE | tag `9.0.3` | AGPLv3 / commercial JUCE licence. Review before distributing. |
| clap-juce-extensions | `7adee3a` (main, first revision with JUCE 9 support) | MIT |

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSNAIRCREATOR_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Options: `-DSNAIRCREATOR_BUILD_CLAP=OFF` skips the CLAP target. Set `SNAIR_SNAPSHOT_DIR=<folder>` when running the tests to write PNG snapshots of the editor states.

Artifacts land in `build/SnairCreator_artefacts/Release/`:
`Standalone/` (app), `VST3/SnairCreator.vst3`, `CLAP/SnairCreator.clap`, and `AU/SnairCreator.component` on macOS.

## CI and releases

`.github/workflows/build.yml` builds, tests and packages on Windows, macOS and Linux for every push and pull request. Pushing a `v*` tag also publishes a GitHub pre-release containing:

- Windows: Inno Setup installer (Standalone + VST3 + CLAP, selectable), portable zip, VST3 zip, CLAP zip
- macOS (universal): `.pkg` installer, portable `.app` zip, VST3, CLAP and AU zips (ad-hoc signed, not notarized)
- Linux: portable standalone, VST3 and CLAP tarballs

## Verification performed for v0.0.1 (Linux, GCC 13.3, CMake 3.28)

- Unit tests: 36 groups, 0 failures (analysis, both engines, determinism, bounds, playback, export formats, presets, processor render/undo/state/missing-source, editor).
- pluginval 1.x, strictness 8 (VST3, including GUI tests under Xvfb): SUCCESS.
- clap-validator 0.3.2: 18 passed, 0 failed, 3 skipped.
- Editor snapshots checked at 1000×700 and 760×520 (main, advanced, help, hit view).
