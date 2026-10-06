# SnairCreator Render Engine Specification

## Purpose

Define the offline analysis-to-render contract for Snare and Clap. The approved design specification defines detailed DSP character, ranges and layer concepts. This document owns module boundaries, render request behavior, determinism and safety.

## Core data contracts

SourceAnalysis contains source duration, channels/rate, peak/RMS/crest, transient positions/density, spectral descriptors, band energy, body-resonance estimate, tonal/noisy estimate, candidate attack/tail windows and deterministic source fingerprint.

RenderRequest is immutable and contains owned source/reference data, SourceAnalysis, complete parameter snapshot, output sample rate, seed, request ID and cancellation token. RenderedHit is immutable sample data plus sample rate, channel count, duration, peak, parameter/source/seed fingerprint and generation ID.

A render is valid only when its request ID and source/parameter generation still match the current request. Stale results are discarded.

## Renderer boundaries and pipeline

SourceAnalyzer computes descriptors only. LayerExtractor produces source-derived attack/body/texture/tail material. SnareRenderer and ClapRenderer create mode-specific layers. SnairEngine maps public parameters to those renderers and combines them. Safety stage checks finite samples, fades edges, enforces peak policy and optional normalization. Renderer returns a complete immutable hit or an actionable failure; it never publishes partial output.

Conceptual order:
source decode → analysis → region extraction → mode-specific layers → envelopes → pitch/filter → reconstruction → stereo → saturation/tone → peak safety → optional normalization → immutable hit.

## Snare behavior

Snare uses attack/crack, body, wire/texture, and optional reinforcement. Source transient supplies attack where useful; body is source-informed but clamped to approved range; wire/texture emphasizes source noise where possible; procedural reinforcement decreases as Source Character rises. Attack duration and frequencies follow the approved design. If a layer cannot be derived, mark fallback use in render metadata and produce bounded result rather than failing silently.

## Clap behavior

Clap has 2–6 deterministic micro-events plus texture/tail. Burst times, amplitudes, filter state, read offsets and stereo placement derive from seed and controls. A tonal source may be filtered/decorrelated/granulated while retaining identity. Burst spacing stays in approved range and must produce distinct events when count exceeds one.

## Parameter mapping

All public IDs and legal ranges come from design spec §12; host-facing ID changes require migration decision. Every parameter maps to documented internal dimensions. Macro controls influence meaningful combinations, not opaque arbitrary multipliers. Mode-specific values either become inactive/disabled or have documented mapping; unrelated controls cannot silently change.

Source Character modifies source contribution and reinforcement/spectral/envelope correction, not simple dry/wet. Output trim is applied at playback or render/export according to one explicit policy. Export controls must not alter the active hit.

## Determinism and request queue

For same decoded source, sample rate, parameter snapshot and seed, output is equivalent within documented platform tolerance. All random choices use deterministic renderer-owned PRNG; no global RNG or realtime random DSP. Changed seed produces a meaningful difference. Rapid parameter movement is debounced/coalesced; at most the current request plus one active worker request need be retained. Cancel stale work safely. Last valid hit remains playable until replacement is complete.

## Output safety

Reject/sanitize non-finite values before active buffer. Apply edge fades where needed. With normalization enabled, pre-output-trim peak is at or below 1.0. Clamp public parameters before scheduling render. Long/noisy/silent inputs cannot create unbounded duration, memory, denormals or runaway peak. Render errors preserve the previous hit and report the actual cause.

## Acceptance criteria

- Unit tests run renderers without UI/host.
- Same inputs/state/seed reproduce; changed seed differs.
- Rapid parameter changes do not queue unbounded obsolete renders.
- Old hit remains playable during render; completed hit swap is safe.
- Snare and Clap generate valid finite non-empty buffers from approved source corpus.
- Difficult-source cases match the source-ingestion policy and remain bounded.
- Safety/normalization/trim rules are verified numerically and by listening.
