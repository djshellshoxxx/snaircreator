# Building SnairCreator

SnairCreator is currently a native standalone JUCE application. Plugin formats are deferred until the standalone workflow is stable.

## Requirements

- CMake 3.24 or newer
- C++20 compiler
- Git
- Windows: Visual Studio 2022 or newer with Desktop development with C++
- macOS: Xcode command line tools and a supported macOS SDK

CMake fetches JUCE 9.0.3 from its official GitHub repository during configuration. Review JUCE's separate license terms for your intended use and distribution model before shipping builds.

## Configure and build

From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

On Windows, launch the resulting `SnairCreator` application from the Release build output. On macOS, open the generated `.app` bundle.

## Current implementation status

The current app shell supports source file selection/drop, WAV/AIFF header validation, source metadata, and Snare/Clap selection. Macro controls, sound generation, playback, randomization, presets, and WAV export are visibly unavailable until their implementation slices are complete.

No native build has been verified yet. Record the operating system, compiler, CMake version, and build output when testing.
