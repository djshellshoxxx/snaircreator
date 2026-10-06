# SnairCreator Design Specification

Status: Approved design; standalone-first implementation in progress  
Product: SnairCreator  
First target: Native standalone desktop application  
Later formats: VST3 and CLAP; AU on macOS after standalone stability  
Framework: JUCE/C++

## 1. Product definition

SnairCreator is a percussion sound-design instrument that transforms an arbitrary audio source into a playable snare or clap while preserving enough of the original source character to make the result unique.

The source can be nearly any decodable audio material: a drum hit, vocal fragment, field recording, machine noise, impact, guitar, vinyl noise, household sound, music excerpt, or other audio. The engine analyzes the source, extracts useful transient, tonal, and noisy components, then reconstructs those components into a controlled percussion hit.

The first delivery is a native standalone desktop app for loading a source, generating a Snare or Clap, previewing it, and exporting WAV. MIDI and DAW session recall belong to later plugin-format work; they must not delay the standalone workflow.

The primary user-facing modes are deliberately simple:

- **Snare**
- **Clap**

There is no separate Hybrid mode in the primary interface. Cross-character behavior is available through layer and blend controls where useful without adding another top-level workflow.

## 2. Product goals

SnairCreator must make unusual source material musically useful quickly.

A successful result should satisfy all of the following:

1. The output is clearly usable as either a snare or clap.
2. The source material still contributes audible character when Source Character is raised.
3. The same source can produce many useful variations without requiring destructive manual editing.
4. Generated sounds can be auditioned and played immediately from MIDI.
5. The exact current sound can be exported to WAV.
6. All expensive analysis and rendering stays outside the realtime audio callback.
7. A project reopened in a host restores the generated sound and parameter state reliably.

## 3. Core user workflow

1. Launch the SnairCreator standalone app.
2. Drag an audio file into the source area or choose Load Source.
3. The plugin decodes and analyzes the source.
4. Choose **Snare** or **Clap**.
5. The engine generates an initial percussion hit automatically.
6. Shape the result with macro controls.
7. Open Advanced controls when deeper editing is required.
8. Use the on-screen Preview button to audition the generated hit. MIDI triggering is deferred to plugin-format work.
9. Use Randomize or Mutate to generate variations.
10. Export the current sound as WAV when desired.
11. Save an internal preset. DAW project recall is deferred to plugin-format work.

The workflow must remain usable without opening the Advanced panel.

## 4. Source handling

### 4.1 Accepted source concept

"Any sound" means any audio source that can be decoded by the target build. The plugin must not claim that arbitrary non-audio files can be transformed.

The first release should support the common formats that JUCE can decode reliably in the configured build. WAV and AIFF are mandatory. Additional formats supported by the deployed JUCE codecs may be enabled where licensing and platform support permit.

### 4.2 Source loading

Source files may be loaded by:

- drag and drop
- file chooser

The source must be decoded and copied into plugin-owned memory. The realtime audio callback must never read the source file from disk.

### 4.3 Source preprocessing

Before analysis:

1. Decode to floating-point samples.
2. Reject empty or invalid buffers.
3. Replace or reject non-finite samples.
4. Convert channel layouts greater than stereo to a controlled mono/stereo analysis representation.
5. Remove insignificant DC offset.
6. Calculate a source peak without destructive normalization.
7. Bound analysis duration for extremely long files.

When a long source is loaded, the engine should scan for useful transient candidates rather than simply analyzing the first few seconds.

## 5. Source analysis

The analyzer produces a reusable `SourceAnalysis` structure. At minimum it contains:

- duration
- channel count
- sample rate
- peak amplitude
- RMS level
- crest factor
- strongest transient position
- multiple transient candidates where available
- transient density
- spectral centroid
- spectral rolloff
- spectral flatness
- zero-crossing rate
- low/mid/high energy ratios
- estimated dominant/body resonance
- noisy-versus-tonal estimate
- attack-window boundaries
- tail/noise candidate regions
- deterministic source fingerprint

The analyzer does not need machine learning in the first release.

## 6. Transformation philosophy

SnairCreator is not a simple filter preset placed over the original file.

The source is decomposed conceptually into reusable percussion components:

- **Attack material**: a short region around a strong onset.
- **Body material**: low and low-mid source energy that can contribute weight and resonance.
- **Texture material**: noisy/high-frequency source content used for snare-wire or clap texture.
- **Tail material**: decaying source-derived texture that can extend the hit naturally.

These components are processed independently, then reconstructed into a percussion event.

This gives the engine two simultaneous goals:

1. preserve identifiable source character;
2. force the timing, envelope, and spectrum toward a useful snare or clap.

## 7. Source Character control

`Source Character` is one of the defining controls of SnairCreator.

Range: `0.0-1.0`

At lower values, the engine applies stronger conventional percussion shaping and relies more heavily on synthesized/resonant reinforcement.

At higher values, more of the original source transient, spectral profile, and source-derived texture remains audible.

The control should influence several internal processes rather than act as a simple dry/wet mix. It may alter:

- attack-source contribution
- body-source contribution
- texture-source contribution
- amount of synthetic reinforcement
- spectral correction strength
- envelope forcing
- transient replacement versus preservation

A separate hidden/internal dry mix may exist for implementation purposes, but Source Character is the user-facing identity control.

## 8. Snare engine

Snare mode reconstructs the source into three primary layers and an optional fourth reinforcement layer.

### 8.1 Attack / crack layer

The engine selects a strong source transient and creates a short attack layer.

Processing may include:

- transient isolation
- very short fade-in/out protection
- pitch shift
- high-pass filtering
- transient emphasis
- spectral tilt
- soft clipping

The attack layer should normally occupy roughly the first 5-40 ms of the hit depending on the source and settings.

### 8.2 Body layer

The body produces weight and drum-like resonance.

The default body frequency is derived from source analysis but clamped into a useful range.

Recommended default range:

`110-320 Hz`

Advanced user range:

`70-450 Hz`

The body may use one or more of:

- filtered source body material
- resonant filter excitation
- short damped sinusoidal resonator
- source-informed modal resonance

The body should not sound like an obvious static sine wave at normal settings.

### 8.3 Wire / noise layer

The snare-wire texture is derived from source noise where possible.

Typical emphasis range:

`1.5-12 kHz`

Processing may include:

- whitening/decorrelation
- high-pass filtering
- band-pass shaping
- source-grain scrambling
- very short randomized read offsets
- envelope shaping

The process must be deterministic for a given source, seed, and parameter state.

### 8.4 Reinforcement layer

When the source lacks useful transient or body energy, a subtle procedural layer may reinforce the hit.

This layer exists to maintain usability for difficult inputs such as sustained pads or nearly silent/noisy material. Its level should fall as Source Character increases.

### 8.5 Snare reconstruction

Conceptually:

`snare = attack + body + wire/texture + optional reinforcement`

The layers then pass through output tone shaping, saturation, limiting/protection, and trim.

## 9. Clap engine

Clap mode builds a clap from several tightly spaced source-derived bursts followed by a decaying texture tail.

### 9.1 Initial clap bursts

The engine produces 2-6 short micro-events.

Default spacing range:

`8-35 ms`

Each micro-event can use a slightly different deterministic read offset, filter state, amplitude, and stereo placement.

The result should avoid mechanical exact repetition.

### 9.2 Clap texture

Useful noisy material is extracted from the source and shaped into the body of the clap.

If the source is strongly tonal, the engine may partially decorrelate, filter, or grain the material to make it more clap-like while preserving spectral identity.

### 9.3 Clap tail

After the initial cluster, a broader filtered tail gives the clap size and cohesion.

Tail processing may include:

- exponential decay
- source-derived noise
- high-pass/band-pass filtering
- stereo decorrelation
- optional diffusion

### 9.4 Clap reconstruction

Conceptually:

`clap = burst1 + burst2 + ... + burstN + texture tail`

## 10. Cross-character blend

Although there is no top-level Hybrid mode, users may introduce selected characteristics of the opposite mode.

A `Blend` or `Layer` control may add:

- clap-style micro-bursts to a snare
- snare-style body reinforcement to a clap

The default is zero. The control should remain secondary or advanced so the primary Snare/Clap workflow stays clear.

## 11. Macro controls

The main GUI exposes six macros.

### Punch

Controls perceived impact.

May map to:

- attack gain
- body gain
- transient envelope
- short saturation

### Snap

Controls upper transient emphasis.

May map to:

- attack brightness
- wire/clap brightness
- transient sharpness
- clap burst emphasis

### Body

Controls low-mid drum weight and resonance.

May map to:

- resonant body amount
- decay
- source-body contribution

### Texture

Controls source-derived noisy character.

May map to:

- wire/noise gain
- clap texture
- source grain contribution
- tail texture

### Dirt

Controls nonlinear character.

May map to:

- saturation
- clipping softness
- source crunch
- harmonic reinforcement

### Size

Controls perceived hit size.

May map to:

- body decay
- texture tail
- clap tail
- subtle diffusion where enabled

Macros must map to stable parameter ranges and must not cause unsafe output levels.

## 12. Advanced parameters

Stable public parameter IDs should be chosen early and not renamed casually because DAW automation depends on them.

Recommended initial set:

- `mode`: snare | clap
- `source_character`: 0-1
- `punch`: 0-1
- `snap`: 0-1
- `body`: 0-1
- `texture`: 0-1
- `dirt`: 0-1
- `size`: 0-1
- `body_freq_hz`: 70-450
- `attack`: 0-1
- `noise`: 0-1
- `tail_ms`: 20-2000
- `pitch_st`: -24 to +24 semitones
- `tone`: -1 to +1
- `drive_db`: 0-24 dB
- `width`: 0-1
- `clap_count`: integer 2-6
- `clap_spread_ms`: 8-35 ms
- `cross_blend`: 0-1
- `output_trim_db`: -24 to +12 dB
- `normalize_render`: boolean
- `seed`: 0-2147483647

Parameters unavailable or meaningless for the current mode should either be disabled visually or mapped in a clearly documented mode-specific way.

## 13. Deterministic generation

Generation must be reproducible.

Given the same:

- decoded source samples
- sample rate
- parameter state
- seed

SnairCreator should produce the same rendered hit within normal floating-point/platform tolerance.

All pseudo-random decisions use a deterministic PRNG owned by the offline render engine.

The realtime playback path must not make random DSP decisions.

## 14. Randomize and Mutate

### Randomize

Randomize creates a substantially different but musically bounded variation.

It may alter:

- seed
- body frequency within safe limits
- source extraction positions
- pitch
- texture amount
- tone
- drive
- size
- clap burst behavior when in Clap mode

Randomize must never create invalid or extreme values outside documented parameter limits.

### Mutate

Mutate creates a smaller variation while preserving the current identity.

Typical parameter movement should remain approximately within 10-20% of useful normalized ranges unless a discrete parameter requires a one-step change.

### Undo Mutation

At least one previous mutation state must be recoverable instantly.

A small bounded mutation history is preferable if simple to implement, but one-level undo is sufficient for the first release.

## 15. Offline render pipeline

A new generated hit is rendered outside the realtime audio callback.

Recommended order:

`source buffer`

`-> source analysis`

`-> transient/body/texture extraction`

`-> mode-specific layer generation`

`-> layer envelopes`

`-> pitch/filter processing`

`-> mode reconstruction`

`-> stereo processing`

`-> saturation`

`-> tone shaping`

`-> safety peak control`

`-> optional normalization`

`-> immutable RenderedHit buffer`

The final `RenderedHit` is then atomically/safely made available to the playback engine.

## 16. Realtime playback architecture

The plugin is an instrument.

The expensive transformation engine does not run per sample in the host audio callback.

`processBlock()` should perform only bounded realtime-safe work such as:

- MIDI event handling
- voice triggering
- immutable rendered-buffer reads
- interpolation if required
- gain/velocity scaling
- voice mixing
- lightweight realtime-safe output processing if explicitly designed

No filesystem access, memory allocation dependent on source size, source analysis, WAV encoding, locks with unbounded wait time, or regeneration may occur in the audio callback.

## 17. MIDI behavior

Any MIDI note may trigger the current generated hit unless a user-selected mapping mode says otherwise.

Defaults:

- note-on triggers playback
- velocity scales amplitude
- note-off does not cut a normal one-shot by default
- up to 16 overlapping voices
- repeated notes may overlap

Recommended convenience mappings:

- C1 / MIDI 36
- D1 / MIDI 38

These are defaults only; SnairCreator should not require a specific drum note.

Future expansion may map different mutation slots across notes, but this is out of scope for the first implementation.

## 18. Preview behavior

The GUI provides a Preview button.

Preview triggers the exact same rendered buffer used by MIDI playback.

This guarantees that the preview, host playback, and exported WAV refer to the same generated sound.

## 19. WAV export

`Export WAV` writes the current `RenderedHit`, not a newly randomized or separately processed render.

Supported export depths:

- 16-bit PCM
- 24-bit PCM
- 32-bit float

Supported channel layouts:

- mono
- stereo

Sample-rate choices:

- current host/render sample rate
- 44.1 kHz
- 48 kHz
- 88.2 kHz
- 96 kHz

If export sample rate differs from the internal render rate, use a high-quality offline resampler.

Optional export controls:

- normalize
- include current output trim
- mono/stereo
- bit depth
- sample rate

Recommended automatic filename pattern:

`SnairCreator_<Mode>_<SourceName>_<Seed>.wav`

Examples:

`SnairCreator_Snare_MetalPipe_0042.wav`

`SnairCreator_Clap_Vocal_0187.wav`

Invalid filename characters must be sanitized safely.

## 20. Presets and state

The plugin must persist enough information for DAW session recall.

State includes:

- complete public parameter state
- selected mode
- seed
- source analysis metadata
- source identity/fingerprint
- source file path when available
- rendered hit data or another reliable recovery representation
- GUI state that materially affects usability

### Session recovery requirement

A reopened DAW project should still produce the same generated hit even if the external source file has moved.

For reliability, the first implementation should serialize or otherwise persist the current rendered hit inside plugin state where host state-size limits permit. The source path may still be stored for re-edit/re-analysis purposes.

If embedding the full original source would make state excessively large, the plugin should embed the rendered output and require the original source only when the user wants to regenerate from that source.

The GUI must clearly report when the original source is unavailable.

## 21. GUI design

Visual family: Circuit Drift Labs.

Direction:

- dark graphite/black base
- restrained cyan/blue technical accents
- limited magenta transient highlights
- high-contrast controls
- clean waveform display
- modern digital instrument appearance
- no faux wood, screws, rack ears, or imitation analog hardware

### 21.1 Main layout

Header:

- SnairCreator logo/name
- version
- About/menu

Source area:

- large drag/drop target
- Load Source button
- source filename
- source duration
- waveform
- compact analysis status

Mode selector:

`SNARE | CLAP`

Primary macro row:

- Punch
- Snap
- Body
- Texture
- Dirt
- Size

Identity control:

- Source Character

Action row:

- Preview
- Randomize
- Mutate
- Undo
- Reset

Bottom/status row:

- MIDI activity
- render status
- Export WAV
- Save Preset
- Load Preset

Advanced panel:

- body frequency
- attack
- noise
- tail
- pitch
- tone
- drive
- width
- clap count
- clap spread
- cross blend
- output trim
- normalization
- seed

### 21.2 Editor sizing

Recommended default:

`1000 x 700`

Minimum:

`760 x 520`

The editor should be resizable with sensible scaling and no overlapping controls.

## 22. Waveform display

The waveform view shows the loaded source and should visually mark the currently selected transient candidate where practical.

Useful overlays:

- source waveform
- detected strongest transient
- selected attack extraction region
- optional generated-hit waveform toggle

The first implementation does not require a full destructive waveform editor.

## 23. Architecture

Recommended repository structure:

```text
native/
  CMakeLists.txt
  Source/
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp
    dsp/
      SourceAnalyzer.h
      SourceAnalyzer.cpp
      SnairEngine.h
      SnairEngine.cpp
      SnareRenderer.h
      SnareRenderer.cpp
      ClapRenderer.h
      ClapRenderer.cpp
      LayerExtraction.h
      LayerExtraction.cpp
      DeterministicRng.h
      RenderedHit.h
    playback/
      HitVoice.h
      HitVoice.cpp
      HitPlayer.h
      HitPlayer.cpp
    state/
      StateSerializer.h
      StateSerializer.cpp
    ui/
      WaveformView.h
      WaveformView.cpp
      MacroKnob.h
      MacroKnob.cpp
  tests/
    SourceAnalyzerTests.cpp
    SnairEngineTests.cpp
    PlaybackTests.cpp
    StateTests.cpp
    ExportTests.cpp
```

Responsibilities must remain separated:

- `SourceAnalyzer`: measurement only
- `LayerExtraction`: source-derived material extraction
- `SnareRenderer`: snare construction
- `ClapRenderer`: clap construction
- `SnairEngine`: orchestration and parameter mapping
- `HitPlayer`: realtime-safe playback only
- `StateSerializer`: persistence
- GUI classes: presentation and user interaction only

## 24. Threading model

At minimum there are three conceptual execution contexts:

### Audio thread

Realtime-safe playback only.

### Message/UI thread

UI updates, chooser interactions, host communication, and lightweight state changes.

### Worker/render thread

Source analysis and generated-hit rendering.

When parameters requiring regeneration change, rendering should be debounced/coalesced where useful so rapid knob movement does not create an unbounded queue of obsolete jobs.

Only the newest relevant render should become active.

## 25. Parameter automation

Musically useful controls should be host automatable.

However, controls that require offline regeneration need deliberate behavior.

Recommended policy:

- macro and advanced generation parameters are automatable in the host
- parameter changes schedule a non-realtime regeneration
- the old rendered hit continues playing until the new hit is ready
- completed render swaps atomically between audio blocks

This is safer than rebuilding the sound inside `processBlock()`.

Export-specific controls such as bit depth do not need host automation.

## 26. Output safety

Every generated buffer must contain finite floating-point samples.

Before becoming active:

1. scan for NaN/Inf
2. calculate peak
3. apply selected normalization policy
4. prevent accidental extreme levels
5. apply short edge fades where necessary to prevent clicks

With normalization enabled, rendered peak must be `<= 1.0` before realtime output trim.

No render may silently emit NaN, infinity, or denormal-driven instability.

## 27. Difficult source behavior

### Silence

Silence must not crash or generate NaN.

Recommended behavior: warn that the source contains insufficient usable energy and either produce a conservative procedural percussion fallback or prevent generation until a useful source is loaded. The implementation should choose one policy consistently and test it.

### Very short source

Never read outside the source buffer. Extraction windows shrink safely.

### Sustained tonal source

Transient detection may fail to find a strong impact. The renderer should synthesize/derive an attack and use the source mainly for body/texture identity.

### Very noisy source

Use transient detection where possible and construct stronger body reinforcement.

### Extremely long source

Bound analysis and retain only required extraction regions/metadata after processing where practical.

## 28. Platform and plugin targets

Required first-class builds:

### Windows

- VST3
- CLAP
- Standalone

### macOS

- VST3
- CLAP
- AU
- Standalone

Linux may be enabled once the native build and CI prove stable, but it is not required to block the initial beta if platform packaging adds disproportionate complexity.

AAX is out of scope.

## 29. Build system

Use CMake with JUCE.

CLAP may use a maintained JUCE CLAP integration such as `clap-juce-extensions` if it remains suitable at implementation time.

Dependencies should be pinned to known revisions/tags in production CI rather than following moving branches.

## 30. Testing strategy

The DSP engine should be testable without instantiating the full GUI.

### 30.1 Source analysis tests

Test:

- silence
- impulse
- sine wave
- noise
- short buffers
- stereo buffers
- invalid/non-finite input sanitation
- transient positioning

### 30.2 Determinism tests

Verify:

- same input + params + seed -> equivalent render
- changed seed -> measurably different render
- Randomize remains within legal ranges
- Mutate remains within bounded variation ranges

### 30.3 Snare tests

Verify:

- finite output
- meaningful attack envelope
- output duration constraints
- body-frequency bounds
- no out-of-range reads

Tests should avoid claiming subjective audio quality from numerical assertions alone.

### 30.4 Clap tests

Verify:

- requested clap-count bounds
- valid burst spacing
- finite output
- deterministic jitter
- stereo output safety

### 30.5 Playback tests

Verify:

- MIDI note starts a voice
- velocity affects amplitude
- overlapping voices work
- voice limit is enforced
- note-off does not truncate normal one-shot playback by default

### 30.6 State tests

Verify:

- save/restore parameters
- mode and seed survive reload
- rendered sound survives host-state serialization strategy
- missing original source is handled explicitly

### 30.7 Export tests

Verify:

- valid WAV headers
- 16-bit PCM export
- 24-bit PCM export
- 32-bit float export
- mono/stereo
- selected sample rate
- export matches the current generated buffer within expected resampling/encoding tolerance

## 31. Manual QA matrix

Test at least:

Sources:

- kick
- vocal
- guitar
- sustained synth
- white noise
- metallic impact
- field recording
- music excerpt
- very quiet recording
- very short click

Modes:

- Snare
- Clap

Actions:

- load
- drag/drop
- preview
- MIDI trigger
- parameter adjustment
- rapid parameter adjustment
- Randomize
- Mutate
- Undo
- mode switch
- save preset
- reload preset
- host project save/reopen
- WAV export

Hosts should include at least one major DAW on Windows and one on macOS before calling the beta broadly usable.

## 32. Performance requirements

The realtime callback must remain lightweight after a hit has been generated.

Targets for the beta:

- no filesystem operations on audio thread
- no source analysis on audio thread
- no source-sized dynamic allocation on audio thread
- no blocking worker-thread join from audio thread
- maximum 16 playback voices by default
- regeneration may take perceptible time for large files but must not interrupt ongoing audio

The GUI should show `Rendering...` when a new hit is being generated.

## 33. Error handling

User-visible failures should be concise and actionable.

Examples:

- unsupported audio format
- file could not be decoded
- source contains no usable audio
- render failed
- export destination unavailable
- original source missing after session reload

Errors must not block the host audio thread.

## 34. Preset concept

Internal presets store synthesis state and may reference a source.

Preset categories may eventually include:

- Tight Snare
- Fat Snare
- Dirty Snare
- Bright Clap
- Wide Clap
- Short Clap

Factory presets should be parameter recipes, not copyrighted third-party samples.

A preset relying on source-specific analysis should clearly indicate when its original source is unavailable.

## 35. Accessibility and interaction

Controls should provide:

- visible labels
- keyboard focus where practical
- numeric value entry for detailed parameters
- reset-to-default interaction
- tooltips for non-obvious controls
- sufficient contrast

Mouse-wheel behavior should avoid accidental large parameter changes.

## 36. Telemetry and privacy

No telemetry, accounts, cloud processing, or source uploads are required.

Loaded audio remains local to the user's machine.

## 37. Out of scope for first beta

- machine-learning source separation
- neural audio generation
- cloud processing
- accounts
- sample marketplace
- AAX
- multi-pad drum rack
- sequencer
- full waveform destructive editor
- automatic stem separation
- tempo-following loop generation
- mobile applications
- multiple simultaneous source files

These may be considered later only after the single-source Snare/Clap workflow is stable.

## 38. Release artifacts

Initial beta release should produce clearly named artifacts for supported platforms.

Examples:

- Windows VST3
- Windows CLAP
- Windows Standalone
- macOS VST3
- macOS CLAP
- macOS AU
- macOS Standalone

CI should also run native unit tests before release artifacts are published.

Code signing/notarization is a packaging concern and should be documented separately from core DSP correctness.

## 39. Acceptance criteria

The first beta is acceptable when all of the following are true:

1. A user can load an arbitrary supported audio file.
2. The plugin analyzes it without blocking realtime playback.
3. Snare mode creates a recognizably snare-like result from varied source classes.
4. Clap mode creates a recognizably clap-like result from varied source classes.
5. Source Character audibly controls how strongly the source identity remains.
6. Macro controls produce useful, bounded changes.
7. Preview plays the active rendered hit.
8. MIDI triggers the same active hit.
9. Randomize creates a new bounded variation.
10. Mutate creates a smaller bounded variation.
11. Undo restores the prior mutation state.
12. DAW automation does not perform expensive rendering directly on the audio callback.
13. Host session reload restores the generated sound and parameter state.
14. WAV export writes the current audible generated sound.
15. 16-bit, 24-bit, and 32-bit-float WAV export work.
16. Snare and Clap renders remain finite and peak-safe.
17. Windows VST3 and CLAP builds pass CI.
18. macOS VST3, CLAP, and AU builds pass CI.
19. Standalone builds run on supported platforms.
20. Automated DSP/state/export tests pass.

## 40. Definition of done

SnairCreator is complete to this specification when the repository contains:

- this approved design specification
- an implementation plan derived from the specification
- JUCE/C++ source
- CMake project
- VST3 target
- CLAP target
- AU target on macOS
- standalone target
- complete Snare renderer
- complete Clap renderer
- source-analysis engine
- deterministic variation engine
- realtime-safe MIDI playback engine
- resizable GUI
- waveform/source panel
- macro and advanced controls
- Randomize/Mutate/Undo
- preset/state persistence
- reliable host-session recall
- WAV export
- automated tests
- CI build/test workflows
- installation/use documentation
- architecture/DSP documentation
- final implementation-versus-spec audit

Implementation should not be considered finished merely because plugin binaries compile. The complete load -> generate -> edit -> MIDI play -> save/reopen -> WAV export workflow must work end to end.