import json
import tempfile
import unittest
from pathlib import Path

from snaircreator.cli import _load_preset, _params, _save_preset, build_parser


class PresetTests(unittest.TestCase):
    def test_preset_loads_and_cli_overrides_it(self):
        with tempfile.TemporaryDirectory() as d:
            preset = Path(d) / 'source.json'
            preset.write_text(json.dumps({
                'schema': 1,
                'product': 'SnairCreator',
                'parameters': {'mode': 'clap', 'seed': 99, 'width': 0.9}
            }), encoding='utf-8')
            ns = build_parser().parse_args(['input.wav', '-o', 'out.wav', '--preset', str(preset), '--width', '0.2'])
            params = _params(ns)
            self.assertEqual(params['mode'], 'clap')
            self.assertEqual(params['seed'], 99)
            self.assertAlmostEqual(params['width'], 0.2)

    def test_save_and_reload_round_trip(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / 'saved.json'
            params = {'mode': 'hybrid', 'seed': 123, 'width': 0.7, 'normalize': True}
            _save_preset(path, params)
            loaded = _load_preset(path)
            self.assertEqual(loaded['mode'], 'hybrid')
            self.assertEqual(loaded['seed'], 123)
            self.assertAlmostEqual(loaded['width'], 0.7)


if __name__ == '__main__':
    unittest.main()
