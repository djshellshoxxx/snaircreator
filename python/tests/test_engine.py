import math
import os
import tempfile
import unittest
import wave
from pathlib import Path

from snaircreator.engine import analyze, render, sanitize_params
from snaircreator.audioio import render_file


def impulse(n=4800):
    x = [0.0] * n
    x[20] = 1.0
    for i in range(21, min(n, 700)):
        x[i] = math.exp(-(i - 20) / 100.0) * (0.35 if i & 1 else -0.35)
    return x


class EngineTests(unittest.TestCase):
    def test_identical_seed_is_deterministic(self):
        p = {"mode": "snare", "seed": 42, "output_ms": 180, "normalize": True}
        self.assertEqual(render(impulse(), 48000, p), render(impulse(), 48000, p))

    def test_different_seeds_change_output(self):
        a = render(impulse(), 48000, {"seed": 1})
        b = render(impulse(), 48000, {"seed": 2})
        self.assertGreater(sum(abs(x - y) for x, y in zip(a, b)), 0.001)

    def test_silence_stays_finite(self):
        info = analyze([0.0] * 128, 48000)
        self.assertTrue(math.isfinite(info.rms))
        out = render([0.0] * 128, 48000, {"mode": "hybrid", "seed": 9})
        self.assertTrue(all(math.isfinite(x) for x in out))

    def test_short_input_is_safe(self):
        out = render([0.2, -0.2], 48000, {"mode": "clap", "clap_count": 6, "clap_spread_ms": 35})
        self.assertGreater(len(out), 2)
        self.assertTrue(all(math.isfinite(x) for x in out))

    def test_params_clamp(self):
        p = sanitize_params({"body_freq_hz": -2, "clap_count": 99, "drive_db": 100, "tone": 5, "output_ms": 99999})
        self.assertEqual(p["body_freq_hz"], 70.0)
        self.assertEqual(p["clap_count"], 6)
        self.assertEqual(p["drive_db"], 18.0)
        self.assertEqual(p["tone"], 1.0)
        self.assertEqual(p["output_ms"], 2000.0)

    def test_normalized_peak_is_bounded(self):
        out = render(impulse(), 48000, {"mode": "hybrid", "drive_db": 18, "normalize": True})
        self.assertLessEqual(max(abs(x) for x in out), 1.0)

    def test_render_file_writes_pcm_wav(self):
        with tempfile.TemporaryDirectory() as d:
            src = Path(d) / "source.wav"
            dst = Path(d) / "hit.wav"
            with wave.open(str(src), "wb") as w:
                w.setnchannels(1); w.setsampwidth(2); w.setframerate(48000)
                data = bytearray()
                for x in impulse(800):
                    v = max(-32768, min(32767, int(x * 32767)))
                    data += int(v).to_bytes(2, "little", signed=True)
                w.writeframes(bytes(data))
            report = render_file(src, dst, {"seed": 2, "output_ms": 80})
            self.assertTrue(dst.exists())
            self.assertGreater(report.frames, 0)
            with wave.open(str(dst), "rb") as w:
                self.assertEqual(w.getframerate(), 48000)
                self.assertEqual(w.getnchannels(), 1)


if __name__ == "__main__":
    unittest.main()
