from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import wave

from .engine import render


@dataclass(frozen=True)
class RenderReport:
    input_path: str
    output_path: str
    sample_rate: int
    frames: int


def _pcm_to_float(data: bytes, sample_width: int, channels: int) -> list[float]:
    if sample_width not in (1, 2, 3, 4):
        raise ValueError(f"unsupported WAV sample width: {sample_width}")
    frame_bytes = sample_width * channels
    out = []
    for frame_start in range(0, len(data) - frame_bytes + 1, frame_bytes):
        acc = 0.0
        for c in range(channels):
            o = frame_start + c * sample_width
            b = data[o:o + sample_width]
            if sample_width == 1:
                v = (b[0] - 128) / 128.0
            else:
                if sample_width == 3:
                    raw = int.from_bytes(b, "little", signed=False)
                    if raw & 0x800000: raw -= 1 << 24
                    v = raw / 8388608.0
                else:
                    raw = int.from_bytes(b, "little", signed=True)
                    v = raw / float(1 << (sample_width * 8 - 1))
            acc += v
        out.append(acc / channels)
    return out


def read_audio(path: str | Path) -> tuple[list[float], int]:
    p = Path(path)
    if p.suffix.lower() == ".wav":
        with wave.open(str(p), "rb") as w:
            channels = w.getnchannels(); width = w.getsampwidth(); rate = w.getframerate(); data = w.readframes(w.getnframes())
        return _pcm_to_float(data, width, channels), rate
    try:
        import soundfile as sf
    except ImportError as exc:
        raise ValueError("non-WAV input requires optional dependency: pip install snaircreator[formats]") from exc
    data, rate = sf.read(str(p), always_2d=True, dtype="float32")
    return data.mean(axis=1).tolist(), int(rate)


def write_wav(path: str | Path, samples, sample_rate: int, bit_depth: int = 24) -> None:
    depth = 24 if bit_depth == 24 else 16
    width = depth // 8
    payload = bytearray()
    scale = (1 << (depth - 1)) - 1
    floor = -(1 << (depth - 1))
    ceil = scale
    for x in samples:
        value = max(floor, min(ceil, int(round(max(-1.0, min(1.0, float(x))) * scale))))
        payload += int(value).to_bytes(width, "little", signed=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1); w.setsampwidth(width); w.setframerate(sample_rate); w.writeframes(bytes(payload))


def render_file(input_path: str | Path, output_path: str | Path, params=None, bit_depth: int = 24) -> RenderReport:
    src, rate = read_audio(input_path)
    hit = render(src, rate, params)
    output = Path(output_path); output.parent.mkdir(parents=True, exist_ok=True)
    write_wav(output, hit, rate, bit_depth)
    return RenderReport(str(input_path), str(output), rate, len(hit))
