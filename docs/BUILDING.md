# Building SnairCreator

## Browser

The browser edition has no build step. Serve `web/` from any static HTTPS host or localhost. GitHub Pages deployment is defined in `.github/workflows/pages.yml`.

For local testing:

```bash
python -m http.server 8080 -d web
```

Open `http://localhost:8080`.

## Python

Requires Python 3.10 or newer.

```bash
python -m pip install -e python
python -m unittest discover -s python/tests -v
snaircreator --help
```

Optional additional file-format support:

```bash
python -m pip install -e 'python[formats]'
```

## Native prerequisites

- CMake 3.22+
- Git
- C++17 compiler
- Windows: Visual Studio 2022 Build Tools/Community with Desktop C++ workload
- macOS: current Xcode command-line tools
- Linux: standard JUCE X11/ALSA development dependencies

JUCE 8.0.6 and `clap-juce-extensions` are fetched by CMake. The latter also fetches its CLAP dependencies.

## Configure

```bash
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
```

## Native engine test

```bash
cmake --build native/build --config Release --target SnairCreatorTests
ctest --test-dir native/build -C Release --output-on-failure
```

## Windows builds

```powershell
cmake --build native/build --config Release --target SnairCreator_VST3 SnairCreator_CLAP SnairCreator_Standalone --parallel 2
```

Artifacts are under `native/build/SnairCreator_artefacts/Release/`.

Typical install locations:

- VST3: `C:\Program Files\Common Files\VST3\`
- CLAP: `C:\Program Files\Common Files\CLAP\` or a host-supported per-user CLAP directory
- Standalone: run the generated executable directly

## macOS builds

```bash
cmake --build native/build --config Release --target SnairCreator_VST3 SnairCreator_CLAP SnairCreator_AU SnairCreator_Standalone --parallel 2
```

Typical install locations:

- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- CLAP: `~/Library/Audio/Plug-Ins/CLAP/`
- AU: `~/Library/Audio/Plug-Ins/Components/`
- Standalone: launch `SnairCreator.app`

Unsigned beta bundles may be quarantined by macOS when downloaded. Production distribution should add a Developer ID certificate, hardened runtime where applicable, and Apple notarization.

## Linux

The CMake project exposes VST3, CLAP and Standalone targets. Install the JUCE platform prerequisites appropriate to the distribution, configure as above, then build:

```bash
cmake --build native/build --config Release --target SnairCreator_VST3 SnairCreator_CLAP SnairCreator_Standalone --parallel 2
```

## GitHub Actions

`web-python-tests.yml` runs Node syntax/tests and Python package/tests.

`native-build.yml` configures on both Windows and macOS, runs `SnairCreatorTests`, then builds and uploads all native formats required on that platform.

`pages.yml` deploys the static browser app from `web/` when `main` changes.
