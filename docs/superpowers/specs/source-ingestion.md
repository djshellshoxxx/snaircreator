# SnairCreator Source Ingestion Specification

## Supported input

The first release accepts one audio file at a time. WAV and AIFF decoding are mandatory. Other formats may be exposed only when the deployed JUCE build supports them on that platform and their licensing/codec distribution is approved. “Any sound” means decodable audio, not arbitrary file types. Do not accept a second source or multi-pad workflow in the first beta.

Input methods are file chooser and drag-and-drop onto the source panel. Loading a new file must not discard the current source/render until the new file has decoded and passed validation.

## Ingestion lifecycle

States: Empty → Choosing/Drop received → Decoding → Validating → Analyzing → Ready to render; any stage may go to Canceled or Error. Preserve prior valid source/render if replacement fails.

1. Validate extension as a hint only; identify actual format from reader.
2. Bound file size, duration, channels and decode allocation before full load.
3. Decode off audio callback into plugin-owned floating-point memory.
4. Sanitize or reject NaN/Inf; record a sanitation warning.
5. Reject empty, unreadable or all-zero sources with actionable message.
6. Preserve source filename, duration, sample rate, channel count, file size and deterministic fingerprint.
7. Convert analysis view to a controlled mono/stereo representation; document the channel downmix rule and retain needed original channels for texture/stereo work.
8. Remove insignificant DC for analysis only; do not destructively rewrite source.
9. Calculate peak/RMS/crest factor and transient candidates across the whole source, including useful late events.
10. Bound analysis duration and memory for long files; report that scanning was bounded.
11. Produce SourceAnalysis asynchronously; generation begins after valid analysis or an explicit documented fallback path.

## Edge behavior

- One-sample/extremely short buffer: clamp extraction windows, never read out of range, either use safe fallback or reject with reason.
- Silence: follow one selected policy consistently (conservative fallback or require new source); never create non-finite output.
- Sustained tonal source: use procedural/transient reinforcement if approved, retaining source-derived body/texture.
- Very noisy source: find transient candidates and reinforce body.
- Multichannel input: never silently discard channels; tell user how it is represented for analysis.
- Long source: scan transient candidates across the file, not only the first window.
- Corrupt/unsupported file: keep existing hit/source; identify decode vs format vs resource error.
- Source file moved after host save: restore the embedded rendered hit; show source unavailable and offer relink, without silent substitution.

## Threading and memory

Decode, file IO, preprocessing and analysis happen on a worker thread. The audio callback never touches the source path, reader, source-sized memory allocation, analyzer or waveform extraction. Worker cancellation is bounded and cannot block host audio. Retain only source data needed for later parameter regeneration/session editing; release temporary buffers after analysis when safe.

## Acceptance criteria

- Load WAV and AIFF via chooser and drag/drop.
- Reject non-audio/corrupt/empty files without losing prior render.
- Handle silent, very short, stereo, non-finite and long files without crash or out-of-range read.
- A late transient in a long source can be selected.
- Decode/analyze never runs in audio callback.
- Missing source after restore leaves saved rendered sound usable and clearly marked.
