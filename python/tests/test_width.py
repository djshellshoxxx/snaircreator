import math
import unittest

from snaircreator.engine import render


class WidthTests(unittest.TestCase):
    def test_width_changes_clap_micro_timing_texture(self):
        source = [1.0 if i == 20 else math.sin(i * .17) * math.exp(-i / 300.0) for i in range(1200)]
        a = render(source, 48000, {"mode":"clap", "seed":77, "width":0, "output_ms":180})
        b = render(source, 48000, {"mode":"clap", "seed":77, "width":1, "output_ms":180})
        self.assertGreater(sum(abs(x-y) for x,y in zip(a,b)), 0.001)


if __name__ == '__main__':
    unittest.main()
