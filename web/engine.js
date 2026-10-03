export const DEFAULTS = Object.freeze({
  mode: 'snare', seed: 1, character: 0.72, body: 0.68, body_freq_hz: 185,
  crack: 0.72, noise: 0.62, tail_ms: 260, clap_count: 4, clap_spread_ms: 18,
  width: 0.35, drive_db: 4, tone: 0, pitch_st: 0, trim_db: 0,
  normalize: true, output_ms: 420
});

const clamp = (v, lo, hi) => Math.min(hi, Math.max(lo, Number.isFinite(Number(v)) ? Number(v) : lo));
const clampInt = (v, lo, hi) => Math.round(clamp(v, lo, hi));

export function sanitizeParams(input = {}) {
  const p = { ...DEFAULTS, ...input };
  p.mode = ['snare', 'clap', 'hybrid'].includes(p.mode) ? p.mode : 'snare';
  p.seed = clampInt(p.seed, 0, 2147483647);
  p.character = clamp(p.character, 0, 1);
  p.body = clamp(p.body, 0, 1);
  p.body_freq_hz = clamp(p.body_freq_hz, 70, 450);
  p.crack = clamp(p.crack, 0, 1);
  p.noise = clamp(p.noise, 0, 1);
  p.tail_ms = clamp(p.tail_ms, 20, 1200);
  p.clap_count = clampInt(p.clap_count, 2, 6);
  p.clap_spread_ms = clamp(p.clap_spread_ms, 8, 35);
  p.width = clamp(p.width, 0, 1);
  p.drive_db = clamp(p.drive_db, 0, 18);
  p.tone = clamp(p.tone, -1, 1);
  p.pitch_st = clamp(p.pitch_st, -24, 24);
  p.trim_db = clamp(p.trim_db, -24, 12);
  p.normalize = Boolean(p.normalize);
  p.output_ms = clamp(p.output_ms, 40, 2000);
  return p;
}

function finiteSample(v) { return Number.isFinite(v) ? v : 0; }

export function analyze(samples, sampleRate) {
  const n = Math.max(0, samples?.length ?? 0);
  if (!n || !Number.isFinite(sampleRate) || sampleRate <= 0) {
    return { frames: 0, duration: 0, peak: 0, rms: 0, zeroCrossingRate: 0, centroidHint: 0, onsetIndex: 0, bodyFreqHint: 185 };
  }
  let sumSq = 0, peak = 0, zc = 0, strongest = 0, strongestDelta = -1, weighted = 0, weight = 0;
  let prev = finiteSample(samples[0]);
  for (let i = 0; i < n; i++) {
    const x = finiteSample(samples[i]);
    const ax = Math.abs(x);
    sumSq += x * x;
    peak = Math.max(peak, ax);
    if (i > 0) {
      if ((x >= 0) !== (prev >= 0)) zc++;
      const d = Math.abs(x - prev);
      if (d > strongestDelta) { strongestDelta = d; strongest = i; }
      weighted += d * i;
      weight += d;
    }
    prev = x;
  }
  const zcr = zc / Math.max(1, n - 1);
  const centroidHint = weight > 0 ? (weighted / weight) / Math.max(1, n - 1) : 0;
  const roughHz = 90 + Math.min(1, zcr * 18) * 260;
  return {
    frames: n,
    duration: n / sampleRate,
    peak,
    rms: Math.sqrt(sumSq / n),
    zeroCrossingRate: zcr,
    centroidHint,
    onsetIndex: strongest,
    bodyFreqHint: clamp(roughHz, 110, 320)
  };
}

function makeRng(seed) {
  let s = (seed >>> 0) || 0x6d2b79f5;
  return () => {
    s += 0x6d2b79f5;
    let t = s;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

function sourceAt(samples, index, pitchRatio = 1) {
  const n = samples.length;
  if (!n) return 0;
  const pos = Math.abs(index * pitchRatio) % n;
  const i0 = Math.floor(pos), i1 = (i0 + 1) % n, f = pos - i0;
  return finiteSample(samples[i0]) * (1 - f) + finiteSample(samples[i1]) * f;
}

export function render(samples, sampleRate, inputParams = {}) {
  const p = sanitizeParams(inputParams);
  const sr = Number.isFinite(sampleRate) && sampleRate > 1000 ? sampleRate : 48000;
  const src = samples?.length ? samples : new Float32Array([0]);
  const analysis = analyze(src, sr);
  const rng = makeRng((p.seed ^ Math.floor(analysis.rms * 1e9) ^ analysis.onsetIndex) >>> 0);
  const length = Math.max(1, Math.round(sr * p.output_ms / 1000));
  const out = new Float32Array(length);
  const pitchRatio = 2 ** (p.pitch_st / 12);
  const bodyHz = inputParams.body_freq_hz == null ? analysis.bodyFreqHint : p.body_freq_hz;
  const attackLen = Math.max(8, Math.round(sr * 0.018));
  const noiseDecay = Math.max(1, sr * p.tail_ms / 1000);
  const clapSpacing = sr * p.clap_spread_ms / 1000;
  const drive = 10 ** (p.drive_db / 20);
  const bright = 0.35 + (p.tone + 1) * 0.325;
  let lastNoise = 0;

  for (let i = 0; i < length; i++) {
    const t = i / sr;
    const bodyEnv = Math.exp(-t / (0.045 + p.body * 0.22));
    const sourceIndex = analysis.onsetIndex + i;
    const source = sourceAt(src, sourceIndex, pitchRatio);
    const transientEnv = i < attackLen ? Math.exp(-i / Math.max(2, attackLen * 0.22)) : 0;
    const bodyLayer = Math.sin(2 * Math.PI * bodyHz * t) * bodyEnv * p.body * 0.55;
    const sourceCrack = source * transientEnv * p.crack * (0.45 + 0.55 * p.character);
    const white = rng() * 2 - 1;
    const hpNoise = white - lastNoise * (1 - bright);
    lastNoise = white;
    const noiseLayer = hpNoise * Math.exp(-i / noiseDecay) * p.noise * 0.38;

    let clapLayer = 0;
    if (p.mode !== 'snare') {
      for (let c = 0; c < p.clap_count; c++) {
        const jitter = (rng() - 0.5) * clapSpacing * 0.32;
        const center = c * clapSpacing + jitter;
        const local = i - center;
        if (local >= 0) {
          const env = Math.exp(-local / Math.max(1, sr * (0.010 + c * 0.002)));
          if (env > 0.001) {
            const texture = sourceAt(src, analysis.onsetIndex + local * (1 + c * 0.013), pitchRatio);
            clapLayer += (texture * p.character + (rng() * 2 - 1) * (1 - p.character)) * env * 0.24;
          }
        }
      }
      const clapTailStart = (p.clap_count - 1) * clapSpacing;
      if (i >= clapTailStart) {
        const lt = i - clapTailStart;
        clapLayer += hpNoise * Math.exp(-lt / Math.max(1, noiseDecay * 0.75)) * p.noise * 0.22;
      }
    }

    let x;
    if (p.mode === 'clap') x = clapLayer + sourceCrack * 0.35;
    else if (p.mode === 'hybrid') x = bodyLayer + sourceCrack + noiseLayer + clapLayer * 0.8;
    else x = bodyLayer + sourceCrack + noiseLayer;

    x = Math.tanh(x * drive);
    out[i] = Number.isFinite(x) ? x : 0;
  }

  const trim = 10 ** (p.trim_db / 20);
  let peak = 0;
  for (let i = 0; i < out.length; i++) { out[i] *= trim; peak = Math.max(peak, Math.abs(out[i])); }
  if (p.normalize && peak > 0) {
    const gain = Math.min(1 / peak, 8);
    for (let i = 0; i < out.length; i++) out[i] = Math.max(-1, Math.min(1, out[i] * gain * 0.98));
  } else {
    for (let i = 0; i < out.length; i++) out[i] = Math.max(-1, Math.min(1, out[i]));
  }
  return out;
}

export function randomizeParams(current = DEFAULTS, seed = Date.now() & 0x7fffffff) {
  const r = makeRng(seed);
  return sanitizeParams({ ...current, seed,
    body: 0.35 + r() * 0.6, crack: 0.35 + r() * 0.65, noise: 0.3 + r() * 0.68,
    tail_ms: 90 + r() * 610, clap_count: 2 + Math.floor(r() * 5), clap_spread_ms: 9 + r() * 24,
    drive_db: r() * 13, tone: r() * 1.7 - 0.85, pitch_st: Math.round(r() * 24 - 12), character: 0.35 + r() * 0.65
  });
}

export function mutateParams(current, seed = ((current?.seed ?? 1) + 1) & 0x7fffffff) {
  const r = makeRng(seed);
  const factor = () => 0.82 + r() * 0.36;
  return sanitizeParams({ ...current, seed,
    body: current.body * factor(), crack: current.crack * factor(), noise: current.noise * factor(),
    tail_ms: current.tail_ms * factor(), clap_spread_ms: current.clap_spread_ms * factor(),
    drive_db: current.drive_db * factor(), tone: current.tone + (r() - 0.5) * 0.25
  });
}
