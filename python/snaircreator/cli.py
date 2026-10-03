from __future__ import annotations

import argparse
from pathlib import Path
import sys

from .audioio import render_file

SUPPORTED = {'.wav', '.flac', '.aif', '.aiff', '.ogg', '.mp3', '.m4a', '.aac'}


def build_parser():
    p = argparse.ArgumentParser(prog='snaircreator', description='Turn audio files into unique snare/clap one-shots.')
    p.add_argument('input', type=Path, help='audio file or directory')
    p.add_argument('-o', '--output', type=Path, required=True, help='output WAV file or output directory')
    p.add_argument('--mode', choices=['snare','clap','hybrid'], default='snare')
    p.add_argument('--seed', type=int, default=1)
    p.add_argument('--character', type=float, default=.72)
    p.add_argument('--body', type=float, default=.68)
    p.add_argument('--body-freq', dest='body_freq_hz', type=float, default=185)
    p.add_argument('--crack', type=float, default=.72)
    p.add_argument('--noise', type=float, default=.62)
    p.add_argument('--tail-ms', type=float, default=260)
    p.add_argument('--clap-count', type=int, default=4)
    p.add_argument('--clap-spread-ms', type=float, default=18)
    p.add_argument('--drive-db', type=float, default=4)
    p.add_argument('--tone', type=float, default=0)
    p.add_argument('--pitch-st', type=float, default=0)
    p.add_argument('--trim-db', type=float, default=0)
    p.add_argument('--output-ms', type=float, default=420)
    p.add_argument('--no-normalize', action='store_true')
    p.add_argument('--bit-depth', choices=[16,24], type=int, default=24)
    p.add_argument('--recursive', action='store_true', help='process subdirectories when input is a directory')
    return p


def _params(ns):
    keys = ['mode','seed','character','body','body_freq_hz','crack','noise','tail_ms','clap_count','clap_spread_ms','drive_db','tone','pitch_st','trim_db','output_ms']
    d = {k:getattr(ns,k) for k in keys}; d['normalize'] = not ns.no_normalize
    return d


def main(argv=None):
    ns = build_parser().parse_args(argv)
    try:
        if ns.input.is_file():
            target = ns.output if ns.output.suffix.lower() == '.wav' else ns.output / f'{ns.input.stem}-{ns.mode}.wav'
            report = render_file(ns.input, target, _params(ns), ns.bit_depth)
            print(f'wrote {report.output_path} ({report.frames} frames @ {report.sample_rate} Hz)')
            return 0
        if not ns.input.is_dir():
            raise ValueError(f'input does not exist: {ns.input}')
        ns.output.mkdir(parents=True, exist_ok=True)
        iterator = ns.input.rglob('*') if ns.recursive else ns.input.glob('*')
        files = [p for p in iterator if p.is_file() and p.suffix.lower() in SUPPORTED]
        if not files: raise ValueError('no supported audio files found')
        for path in files:
            rel = path.relative_to(ns.input)
            out_dir = ns.output / rel.parent; out_dir.mkdir(parents=True, exist_ok=True)
            out = out_dir / f'{path.stem}-{ns.mode}.wav'
            report = render_file(path, out, _params(ns), ns.bit_depth)
            print(f'wrote {report.output_path}')
        return 0
    except Exception as exc:
        print(f'snaircreator: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
