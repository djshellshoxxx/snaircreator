from __future__ import annotations

from dataclasses import dataclass
import math
from typing import Iterable, Mapping

DEFAULTS = {
    "mode": "snare", "seed": 1, "character": 0.72, "body": 0.68, "body_freq_hz": 185.0,
    "crack": 0.72, "noise": 0.62, "tail_ms": 260.0, "clap_count": 4, "clap_spread_ms": 18.0,
    "width": 0.35, "drive_db": 4.0, "tone": 0.0, "pitch_st": 0.0, "trim_db": 0.0,
    "normalize": True, "output_ms": 420.0,
}


def _clamp(v, lo, hi):
    try: x = float(v)
    except (TypeError, ValueError): x = lo
    if not math.isfinite(x): x = lo
    return min(hi, max(lo, x))


def sanitize_params(values: Mapping | None = None) -> dict:
    p = {**DEFAULTS, **dict(values or {})}
    p["mode"] = p["mode"] if p["mode"] in {"snare", "clap", "hybrid"} else "snare"
    p["seed"] = int(round(_clamp(p["seed"], 0, 2147483647)))
    for key in ("character", "body", "crack", "noise", "width"): p[key] = _clamp(p[key], 0, 1)
    p["body_freq_hz"] = _clamp(p["body_freq_hz"], 70, 450); p["tail_ms"] = _clamp(p["tail_ms"], 20, 1200)
    p["clap_count"] = int(round(_clamp(p["clap_count"], 2, 6))); p["clap_spread_ms"] = _clamp(p["clap_spread_ms"], 8, 35)
    p["drive_db"] = _clamp(p["drive_db"], 0, 18); p["tone"] = _clamp(p["tone"], -1, 1)
    p["pitch_st"] = _clamp(p["pitch_st"], -24, 24); p["trim_db"] = _clamp(p["trim_db"], -24, 12)
    p["output_ms"] = _clamp(p["output_ms"], 40, 2000); p["normalize"] = bool(p["normalize"])
    return p


@dataclass(frozen=True)
class Analysis:
    frames: int; duration: float; peak: float; rms: float; zero_crossing_rate: float; onset_index: int; body_freq_hint: float


def _finite(x):
    try: v = float(x)
    except (TypeError, ValueError): return 0.0
    return v if math.isfinite(v) else 0.0


def analyze(samples: Iterable[float], sample_rate: int) -> Analysis:
    xs = [_finite(x) for x in samples]
    if not xs or sample_rate <= 0: return Analysis(0, 0.0, 0.0, 0.0, 0.0, 0, 185.0)
    peak = ss = 0.0; zc = onset = 0; strongest = -1.0; prev = xs[0]
    for i, x in enumerate(xs):
        peak = max(peak, abs(x)); ss += x * x
        if i:
            if (x >= 0) != (prev >= 0): zc += 1
            d = abs(x - prev)
            if d > strongest: strongest, onset = d, i
        prev = x
    zcr = zc / max(1, len(xs)-1); rough = 90 + min(1.0, zcr * 18) * 260
    return Analysis(len(xs), len(xs)/sample_rate, peak, math.sqrt(ss/len(xs)), zcr, onset, _clamp(rough, 110, 320))


class _Rng:
    def __init__(self, seed): self.state = (int(seed) & 0xffffffff) or 0x6D2B79F5
    def next(self):
        self.state = (self.state + 0x6D2B79F5) & 0xffffffff; t = self.state
        t = ((t ^ (t >> 15)) * (t | 1)) & 0xffffffff
        t ^= (t + (((t ^ (t >> 7)) * (t | 61)) & 0xffffffff)) & 0xffffffff
        return ((t ^ (t >> 14)) & 0xffffffff) / 4294967296.0


def _source_at(xs, index, pitch=1.0):
    if not xs: return 0.0
    pos = abs(index * pitch) % len(xs); i0 = int(pos); i1 = (i0 + 1) % len(xs); f = pos - i0
    return xs[i0] * (1-f) + xs[i1] * f


def render(samples: Iterable[float], sample_rate: int, params: Mapping | None = None) -> list[float]:
    p = sanitize_params(params); sr = sample_rate if sample_rate and sample_rate > 1000 else 48000
    xs = [_finite(x) for x in samples] or [0.0]; a = analyze(xs, sr)
    rng = _Rng((p["seed"] ^ int(a.rms*1_000_000_000) ^ a.onset_index) & 0xffffffff)
    n = max(1, round(sr * p["output_ms"] / 1000)); out = [0.0] * n; pitch = 2 ** (p["pitch_st"] / 12)
    body_hz = p["body_freq_hz"] if params and "body_freq_hz" in params else a.body_freq_hint
    attack = max(8, round(sr*.018)); noise_decay = max(1.0, sr*p["tail_ms"]/1000); spacing = sr*p["clap_spread_ms"]/1000
    drive = 10 ** (p["drive_db"]/20); bright = .35 + (p["tone"]+1)*.325; width_jitter = .08 + p["width"]*.48; last_noise = 0.0
    for i in range(n):
        t = i/sr; body_env = math.exp(-t/(.045+p["body"]*.22)); src = _source_at(xs, a.onset_index+i, pitch)
        transient = math.exp(-i/max(2.0, attack*.22)) if i < attack else 0.0
        body = math.sin(2*math.pi*body_hz*t)*body_env*p["body"]*.55
        crack = src*transient*p["crack"]*(.45+.55*p["character"])
        white = rng.next()*2-1; hp = white-last_noise*(1-bright); last_noise = white
        noise = hp*math.exp(-i/noise_decay)*p["noise"]*.38; clap = 0.0
        if p["mode"] != "snare":
            for c in range(p["clap_count"]):
                jitter = (rng.next()-.5)*spacing*width_jitter; local = i-(c*spacing+jitter)
                if local >= 0:
                    env = math.exp(-local/max(1.0,sr*(.010+c*.002)))
                    if env > .001:
                        texture = _source_at(xs, a.onset_index+local*(1+c*.013), pitch)
                        polarity = -1.0 if (c & 1 and p["width"] > .5) else 1.0
                        clap += (texture*p["character"] + (rng.next()*2-1)*(1-p["character"])) * env * .24 * polarity
            tail_start = (p["clap_count"]-1)*spacing
            if i >= tail_start: clap += hp*math.exp(-(i-tail_start)/max(1.0,noise_decay*.75))*p["noise"]*(.18+.08*p["width"])
        raw = clap+crack*.35 if p["mode"]=="clap" else body+crack+noise+clap*.8 if p["mode"]=="hybrid" else body+crack+noise
        out[i] = math.tanh(raw*drive)
    trim = 10 ** (p["trim_db"]/20); out = [max(-1.0,min(1.0,x*trim)) for x in out]; peak = max((abs(x) for x in out), default=0)
    if p["normalize"] and peak > 0:
        gain = min(1.0/peak, 8.0)*.98; out = [max(-1.0,min(1.0,x*gain)) for x in out]
    return out
