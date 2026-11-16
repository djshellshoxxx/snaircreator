# Developer Guide

Follow `BUILDING.md`. In short:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSNAIRCREATOR_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

- CMake fetches JUCE 9.0.3 (AGPLv3 or commercial licence; review before distributing) and clap-juce-extensions at a pinned revision.
- Linux needs the X11, ALSA and related packages listed in `BUILDING.md`.
- `-DSNAIRCREATOR_BUILD_CLAP=OFF` skips the CLAP target.
- Test snapshots: set `SNAIR_SNAPSHOT_DIR` to write editor PNGs.

Not built or tested in this documentation pass (the JUCE fetch and native build were not run).
